// computer_vision_basic.cpp
//
// Minimal ROS2 node: Subscribes to RealSense camera topics from realsense2_camera node
// Uses ONNX Runtime for robust YOLO inference
//
// Dependencies: ROS2, OpenCV, sensor_msgs, cv_bridge, ONNX Runtime
// Build with ament_cmake or colcon
//
// YOLO26n output format: (1, 300, 6) — NMS-free one-to-one head
// Each detection: [x1, y1, x2, y2, conf, class_id] in letterboxed pixel space

#include <cstdint>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
// ONNX Runtime
#include <onnxruntime_cxx_api.h>
// ROS client includes
#include <ratio>
#include <rclcpp/logging.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <geometry_msgs/msg/detail/vector3__struct.hpp>
// Standard libraries
#include <functional>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <string>
#include <filesystem>
#include <cstring>
#include <ament_index_cpp/get_package_share_directory.hpp>

using namespace std::chrono_literals;
using namespace std;

static const std::vector<std::string> CLASS_NAMES = {
    "person","bicycle","car","motorcycle","airplane","bus","train","truck",
    "boat","traffic light","fire hydrant","stop sign","parking meter","bench",
    "bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe",
    "backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard",
    "sports ball","kite","baseball bat","baseball glove","skateboard","surfboard",
    "tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl",
    "banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza",
    "donut","cake","chair","couch","potted plant","bed","dining table","toilet",
    "tv","laptop","mouse","remote","keyboard","cell phone","microwave","oven",
    "toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear",
    "hair drier","toothbrush"
};

class ComputerVision : public rclcpp::Node
{
public:
    ComputerVision() : Node("computer_vision")
    {
        RCLCPP_INFO(this->get_logger(), "Computer Vision node started");

        // Fetch the ONNX model at launch
        std::string model_path;
        try {
            model_path = ament_index_cpp::get_package_share_directory("snappy_cpp") + "/models/yolo26n.onnx";
        } catch (const std::exception & e) {
            RCLCPP_WARN(this->get_logger(), "Could not get package share directory: %s", e.what());
            model_path = "yolo26n.onnx"; // fallback to relative path
        }

        RCLCPP_INFO(this->get_logger(), "Attempting to load ONNX model from: %s", model_path.c_str());

        if (!std::filesystem::exists(model_path)) {
            RCLCPP_ERROR(this->get_logger(), "ONNX model not found at: %s", model_path.c_str());
            throw std::runtime_error(std::string("ONNX model not found: ") + model_path);
        }

        try {
            Ort::SessionOptions session_options;
            session_options.SetIntraOpNumThreads(NUM_THREADS);
            session_options.SetGraphOptimizationLevel(ORT_ENABLE_EXTENDED);
            ort_session_ = std::make_unique<Ort::Session>(ort_env_, model_path.c_str(), session_options);
            RCLCPP_INFO(this->get_logger(), "ONNX Runtime session created successfully");

            num_classes_ = 80; // COCO pretrained

            // Cache input/output names at construction time (they don't change per frame)
            auto in_ptr  = ort_session_->GetInputNameAllocated(0, ort_allocator_);
            auto out_ptr = ort_session_->GetOutputNameAllocated(0, ort_allocator_);
            input_name_  = std::string(in_ptr.get());
            output_name_ = std::string(out_ptr.get());

            RCLCPP_INFO(this->get_logger(), "Input name: %s | Output name: %s",
                input_name_.c_str(), output_name_.c_str());

        } catch (const std::exception & e) {
            RCLCPP_ERROR(this->get_logger(), "ONNX Runtime exception when creating session: %s", e.what());
            throw;
        }

        // Subscribe to RealSense camera topics
        color_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/color/image_raw", 10,
            std::bind(&ComputerVision::color_callback, this, std::placeholders::_1));

        depth_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/depth/image_rect_raw", 10,
            std::bind(&ComputerVision::depth_callback, this, std::placeholders::_1));

        display_timer_ = this->create_wall_timer(33ms,  std::bind(&ComputerVision::display_callback, this));
        process_timer_ = this->create_wall_timer(100ms, std::bind(&ComputerVision::process_callback, this));
    }

private:
    // State
    uint64_t frame_id_            = 0;
    uint64_t last_processed_frame_id_ = 0;

    cv::Mat latest_color_;
    cv::Mat latest_depth_;
    cv::Mat latest_detection_;

    std::mutex color_mutex_;
    std::mutex depth_mutex_;
    std::mutex detection_mutex_;

    // ONNX Runtime
    Ort::Env ort_env_{ORT_LOGGING_LEVEL_WARNING, "snappy_cv"};
    std::unique_ptr<Ort::Session> ort_session_;
    Ort::AllocatorWithDefaultOptions ort_allocator_;
    int num_classes_ = 80;

    // Cached input/output names (retrieved once in constructor)
    std::string input_name_;
    std::string output_name_;

    // Detection struct
    struct Detection {
        cv::Rect2f box;   // x, y, width, height in original image pixels
        float conf;
        int   class_id;
    };

    // ROS members
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr color_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depth_sub_;
    rclcpp::TimerBase::SharedPtr display_timer_;
    rclcpp::TimerBase::SharedPtr process_timer_;

    // Constants
    static constexpr int   INPUT_HW    = 640;
    static constexpr float CONF_THRESH = 0.25f;  // lowered from 0.75 — YOLO26n NMS-free head
    static constexpr float NMS_THRESH  = 0.45f;  // kept for reference; not used (NMS-free model)
    static constexpr int   NUM_THREADS = 4;

    void color_callback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        try {
            auto cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
            std::lock_guard<std::mutex> lock(color_mutex_);
            latest_color_ = cv_ptr->image.clone();
            frame_id_++;
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void depth_callback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        try {
            auto cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::TYPE_16UC1);
            std::lock_guard<std::mutex> lock(depth_mutex_);
            latest_depth_ = cv_ptr->image.clone();
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void display_callback()
    {
        cv::Mat color, depth, detection;
        {
            std::lock_guard<std::mutex> lock(color_mutex_);
            color = latest_color_.clone();
        }
        {
            std::lock_guard<std::mutex> lock(depth_mutex_);
            depth = latest_depth_.clone();
        }
        {
            std::lock_guard<std::mutex> lock(detection_mutex_);
            detection = latest_detection_.clone();
        }

        if (!color.empty())     cv::imshow("Color Stream", color);
        if (!depth.empty())     show_depth(depth);
        if (!detection.empty()) cv::imshow("Detections", detection);

        cv::waitKey(1);
    }

    void process_callback()
    {
        cv::Mat color;
        uint64_t fid;
        {
            std::lock_guard<std::mutex> lock(color_mutex_);
            if (latest_color_.empty()) return;
            if (frame_id_ == last_processed_frame_id_) return;
            color = latest_color_.clone();
            fid   = frame_id_;
        }
        last_processed_frame_id_ = fid;
        run_ObjectDetectionNonCudaFromStream(color);
    }

    void show_depth(const cv::Mat& depth_raw)
    {
        cv::Mat depth_8u, depth_color_map;
        depth_raw.convertTo(depth_8u, CV_8U, 255.0 / 10000.0);
        cv::applyColorMap(depth_8u, depth_color_map, cv::COLORMAP_JET);
        cv::imshow("Depth Stream", depth_color_map);
    }

    cv::Mat letterboxFeed(const cv::Mat& src, int tw, int th,
                          float& scale, int& pad_x, int& pad_y)
    {
        float ratio     = std::min(float(tw) / src.cols, float(th) / src.rows);
        int   newWidth  = int(src.cols * ratio);
        int   newHeight = int(src.rows * ratio);

        cv::Mat resized;
        cv::resize(src, resized, {newWidth, newHeight}, 0, 0, cv::INTER_LINEAR);

        pad_x = (tw - newWidth)  / 2;
        pad_y = (th - newHeight) / 2;
        scale = ratio;

        RCLCPP_DEBUG(this->get_logger(),
            "Letterbox: orig=%dx%d -> scaled=%dx%d, scale=%.3f, pad=(%d,%d)",
            src.cols, src.rows, newWidth, newHeight, scale, pad_x, pad_y);

        cv::Mat out(th, tw, CV_8UC3, cv::Scalar(114, 114, 114));
        resized.copyTo(out(cv::Rect(pad_x, pad_y, newWidth, newHeight)));
        return out;
    }

    std::vector<Detection> parse_detections(const cv::Mat& output,
        int originalWidth, int originalHeight, float scale, int paddingX, int paddingY)
    {
        std::vector<Detection> result;
        result.reserve(300);

        for (int i = 0; i < output.rows; ++i) {
            float conf = output.at<float>(i, 4);
            if (conf < CONF_THRESH) continue;

            int class_id = (int)output.at<float>(i, 5);
            if (class_id < 0 || class_id >= num_classes_) continue;

            // YOLO26n default export: [x1, y1, x2, y2] already in corner format,
            // in letterboxed 640x640 pixel space — do NOT treat as center format.
            float x1 = output.at<float>(i, 0);
            float y1 = output.at<float>(i, 1);
            float x2 = output.at<float>(i, 2);
            float y2 = output.at<float>(i, 3);

            RCLCPP_DEBUG(this->get_logger(),
                "Raw corners (letterbox px): x1=%.1f y1=%.1f x2=%.1f y2=%.1f",
                x1, y1, x2, y2);

            // Remove letterbox padding, then scale back to original image coords
            x1 = (x1 - paddingX) / scale;
            y1 = (y1 - paddingY) / scale;
            x2 = (x2 - paddingX) / scale;
            y2 = (y2 - paddingY) / scale;

            // Clamp to original image bounds
            x1 = std::clamp(x1, 0.f, (float)originalWidth);
            y1 = std::clamp(y1, 0.f, (float)originalHeight);
            x2 = std::clamp(x2, 0.f, (float)originalWidth);
            y2 = std::clamp(y2, 0.f, (float)originalHeight);

            if (x2 <= x1 || y2 <= y1) continue;

            float bw = x2 - x1;
            float bh = y2 - y1;

            RCLCPP_INFO(this->get_logger(),
                "✓ Detection: %s | conf=%.2f | box=[%.0f, %.0f, %.0f, %.0f]",
                CLASS_NAMES[class_id].c_str(), conf, x1, y1, bw, bh);

            result.push_back({{x1, y1, bw, bh}, conf, class_id});
        }

        if (!result.empty()) {
            RCLCPP_INFO(this->get_logger(), "=== %zu detection(s) this frame ===", result.size());
        }

        return result;
    }

    void draw_detections(cv::Mat& frame, const std::vector<Detection>& detections)
    {
        for (const auto& d : detections) {
            int x = (int)d.box.x;
            int y = (int)d.box.y;
            int w = (int)d.box.width;
            int h = (int)d.box.height;

            cv::Rect rect(x, y, w, h);
            cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);

            std::string label = CLASS_NAMES[d.class_id] + " " +
                                std::to_string((int)(d.conf * 100)) + "%";

            int baseLine = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseLine);

            int textX = x;
            int textY = y - 5;

            cv::rectangle(frame,
                cv::Point(textX, textY - textSize.height - baseLine - 4),
                cv::Point(textX + textSize.width + 4, textY + baseLine),
                cv::Scalar(0, 255, 0), cv::FILLED);

            cv::putText(frame, label, cv::Point(textX + 2, textY - baseLine - 2),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
        }
    }

    void run_ObjectDetectionNonCudaFromStream(const cv::Mat& cameraFrame)
    {
        if (cameraFrame.empty()) return;

        // 1. Letterbox to 640x640
        float scale;
        int   padX, padY;
        cv::Mat letterboxed = letterboxFeed(cameraFrame, INPUT_HW, INPUT_HW, scale, padX, padY);

        // 2. BGR -> RGB, normalize to [0,1], produce NCHW blob
        cv::Mat blob = cv::dnn::blobFromImage(letterboxed, 1.0 / 255.0,
            cv::Size(INPUT_HW, INPUT_HW), cv::Scalar(0, 0, 0), /*swapRB=*/true, false);

        // Ensure contiguous float32
        cv::Mat blob_f;
        if (!(blob.isContinuous() && blob.type() == CV_32F)) {
            blob.convertTo(blob_f, CV_32F);
        } else {
            blob_f = blob;
        }

        int64_t n = blob_f.size[0];
        int64_t c = blob_f.size[1];
        int64_t h = blob_f.size[2];
        int64_t w = blob_f.size[3];
        std::vector<int64_t> input_shape = {n, c, h, w};
        size_t input_tensor_size = static_cast<size_t>(n * c * h * w);

        std::vector<float> input_tensor_values(
            blob_f.ptr<float>(),
            blob_f.ptr<float>() + input_tensor_size);

        // 3. Build ONNX input tensor
        Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor  = Ort::Value::CreateTensor<float>(
            mem_info,
            input_tensor_values.data(), input_tensor_size,
            input_shape.data(), input_shape.size());

        const char* input_name  = input_name_.c_str();
        const char* output_name = output_name_.c_str();

        // 4. Run inference
        auto t0 = std::chrono::steady_clock::now();
        auto output_tensors = ort_session_->Run(
            Ort::RunOptions{nullptr},
            &input_name,  &input_tensor,  1,
            &output_name, 1);
        auto t1 = std::chrono::steady_clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

        // 5. Unpack output — YOLO26n: (1, 300, 6)
        float* out_ptr  = output_tensors[0].GetTensorMutableData<float>();
        auto   out_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        std::vector<int64_t> out_shape = out_info.GetShape();

        RCLCPP_DEBUG(this->get_logger(), "Output shape: [%ld, %ld, %ld]",
            out_shape[0], out_shape[1], out_shape[2]);

        int rows = (int)out_shape[1]; // 300
        int cols = (int)out_shape[2]; // 6

        cv::Mat output(rows, cols, CV_32F);
        std::memcpy(output.data, out_ptr, rows * cols * sizeof(float));

        // 6. Parse and draw
        std::vector<Detection> detections = parse_detections(
            output, cameraFrame.cols, cameraFrame.rows, scale, padX, padY);

        cv::Mat annotated = cameraFrame.clone();
        draw_detections(annotated, detections);

        std::string info = "Infer: " + std::to_string((int)ms) + "ms"
                         + "  det: " + std::to_string(detections.size());
        cv::putText(annotated, info, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        // 7. Hand off to display thread
        {
            std::lock_guard<std::mutex> lock(detection_mutex_);
            latest_detection_ = annotated;
        }
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ComputerVision>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
