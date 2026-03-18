// computer_vision_basic.cpp
//
// Minimal ROS2 node: Subscribes to RealSense camera topics from realsense2_camera node
// Skeleton code for ONNX vision inference (commented).
//
// Dependencies: ROS2, OpenCV, sensor_msgs, cv_bridge
// Build with ament_cmake or colcon
//

//include cv
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>


// ros client inlcude
#include <ratio>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <geometry_msgs/msg/detail/vector3__struct.hpp>
// #include <onnxruntime_cxx_api.h>

// include standard libraries
#include <functional>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>

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


        // fetch the ONNX model at launch
        const std::string model_path = "example.onnx";
        net_ = cv::dnn::readNetFromONNX(model_path);

        net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net_.setPreferableBackend(cv::dnn::DNN_TARGET_CPU);


        // Subscribe to RealSense camera topics
        // color camera feed subscription of realsenseCamera Topic
        color_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/color/image_rect_raw", 10,
            std::bind(&ComputerVision::color_callback, this, std::placeholders::_1));

        //depth feed of camera
        depth_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/depth/image_rect_raw", 10,
            std::bind(&ComputerVision::depth_callback, this, std::placeholders::_1));

        //timers
        display_timer_ = this->create_wall_timer(33ms, std::bind(&ComputerVision::display_callback, this));
        process_timer_ = this->create_wall_timer(100ms, std::bind(&ComputerVision::process_callback, this));
    }

private:
    // variables
    cv::Mat latest_color_;
    cv::Mat latest_depth_;

    std::mutex color_mutex_;
    std::mutex depth_mutex_;

    cv::dnn::Net net_; //this is here to hold the onnx model, and reuse it for each frame
    int num_classes_ = 0;

    //struct for Detection
    struct Detection {
        cv::Rect2f box;
        float conf;
        int class_id;
    };

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr color_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depth_sub_;

    // constexpr for model input size
    static constexpr int INPUT_HW = 640;
    static constexpr float conf_thresh = 0.25f;
    static constexpr float NMS_THRESH = 0.45f;
    static constexpr int NUM_THREADS = 4;


    //timer
    rclcpp::TimerBase::SharedPtr display_timer_;
    rclcpp::TimerBase::SharedPtr process_timer_;

    void timer_callback()
    {
        RCLCPP_INFO(this->get_logger(), "Computer Vision running...");
    }

    void color_callback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        try {
            auto cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
            std::lock_guard<std::mutex> lock(color_mutex_);
            latest_color_ = cv_ptr->image.clone();
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

    void display_callback() {
        cv::Mat color, depth;
        {
            std::lock_guard<std::mutex> lock(color_mutex_);
            color = latest_color_.clone();
        }
        {
            std::lock_guard<std::mutex> lock(depth_mutex_);
            depth = latest_depth_.clone();
        }

        if(!color.empty()) show_color(color);
        if(!depth.empty()) show_depth(depth);

        cv::waitKey(1);
    }


    void show_color(const cv::Mat& cameraFrame) {
        cv::imshow("Color Stream", cameraFrame);
    }

    void show_depth(const cv::Mat& depth_raw) {
        cv::Mat depth_8u, depth_color_map;
        depth_raw.convertTo(depth_8u, CV_8U, 255.0/10000.0);
        cv::applyColorMap(depth_8u, depth_color_map, cv::COLORMAP_JET);
        cv::imshow("Depth Stream", depth_color_map);
    }

    void process_callback() {
        cv::Mat color;
        {
            std::lock_guard<std::mutex> lock(color_mutex_);
            if(latest_color_.empty()) return;
            color = latest_color_.clone();
        }
        run_ObjectDetectionNonCudaFromStream(color);
    }

    cv::Mat letterboxFeed(const cv::Mat& src, int tw, int th, float& scale, int& pad_x, int& pad_y) {
        float ratio = std::min(float(tw)/ src.cols, float(th) / src.rows);

        int newWidth = int(src.cols * ratio);
        int newHeight = int(src.rows * ratio);

        cv::Mat newSize;
        cv::resize(src, newSize, {newWidth, newHeight}, 0, 0, cv::INTER_LINEAR);

        pad_x = (tw - newWidth) / 2 ;
        pad_y = (th - newHeight) / 2 ;

        scale = ratio;

       cv::Mat out(th, tw, CV_8UC3, cv::Scalar(114, 114, 114));
       newSize.copyTo(out(cv::Rect(pad_x, pad_y, newWidth, newHeight)));

       return out;
    }

    std::vector<Detection> parse_detections(const cv::Mat& output, int originalWidth, int originalHeight, float scale, int paddingX, int paddingY) {
        cv::Mat mat = output.reshape(1, output.size[1]);

        int num_anchors = mat.cols;

        std::vector<Detection> result;
        result.reserve(64);

        for(int x =0; x < num_anchors; ++x) {
           float cx = mat.at<float>(0, x);
           float cy = mat.at<float>(1, x);
           float bw = mat.at<float>(2, x);
           float bh = mat.at<float>(3, x);

           int bestClass = 0;
           float bestConfidence = 0.0f;

           for (int y = 0; y < num_classes_; ++y) {
               float confidence = mat.at<float>(4 + y, x);
               if (confidence > bestConfidence) {
                   bestConfidence = confidence;
                   bestClass = y;
               }
           }

           if(bestConfidence < conf_thresh) continue;

           //undo letter box
           float x1 = std::clamp(((cx - bw * 0.5f) - paddingX)/ scale, 0.f, float(originalWidth));
           float y1 = std::clamp(((cy - bh * 0.5f) - paddingY)/ scale, 0.f, float(originalHeight));
           float x2 = std::clamp(((cx + bw * 0.5f) - paddingX)/ scale, 0.f, float(originalWidth));
           float y2 = std::clamp(((cy + bh * 0.5f) - paddingY)/ scale, 0.f, float(originalHeight));

           result.push_back({{x1, y1, x2 - x1, y2 - y1}, bestConfidence, bestClass});
        }

        return result;
    }

    void draw_detections(cv::Mat& frame, const std::vector<Detection>& detections) {
        for(const auto& d: detections) {
            cv::Rect rectangle(d.box);
            cv::rectangle(frame, rectangle, {0, 255, 0}, 2);

            std::string label = (d.class_id < (int)CLASS_NAMES.size() ? CLASS_NAMES[d.class_id] : std::to_string(d.class_id)) + " " + std::to_string(int(d.conf * 100)) + "%";

            int baseLine;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.55, 1, &baseLine);
            cv::rectangle(frame, {rectangle.x, rectangle.y - textSize.height - 6}, {textSize.width + 4, textSize.height + 6}, {0, 255, 0}, cv::FILLED);
            cv::putText(frame, label, {rectangle.x + 2, rectangle.y - 5}, cv::FONT_HERSHEY_SIMPLEX, 0.55, {0, 0, 0}, cv::LINE_AA);

        }
    }

    void run_ObjectDetectionNonCudaFromStream(const cv::Mat& cameraFrame) {
        if(cameraFrame.empty()) return;

        float scale;
        int padX, padY;
        cv::Mat letterboxed = letterboxFeed(cameraFrame, INPUT_HW, INPUT_HW, scale, padX, padY);


        // this will swap channels from BGR to RGB
        cv::Mat blob = cv::dnn::blobFromImage(letterboxed, 1.0/255.0, cv::Size(INPUT_HW, INPUT_HW), cv::Scalar(0,0,0), true, false);

        net_.setInput(blob);
        auto t0 = std::chrono::steady_clock::now();
        cv::Mat output = net_.forward();
        auto t1 = std::chrono::steady_clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

        std::vector<Detection> detections = parse_detections(output, cameraFrame.cols, cameraFrame.rows, scale, padX, padY);

        cv::Mat cameraClone = cameraFrame.clone();
        draw_detections(cameraClone, detections);

        std::string info = "Infer: " + std::to_string(int(ms)) + "ms" + " det: " + std::to_string(detections.size());

        cv::putText(cameraFrame, info, {10, 30}, cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        cv::imshow("Detections", cameraFrame);
        cv::waitKey(1);
    };


};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ComputerVision>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
