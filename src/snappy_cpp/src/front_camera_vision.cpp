// front_camera_vision.cpp
// Headless YOLOv26 segmentation inference node for D455 camera
// Publishes DetectionArray for external visualization nodes

#include <rclcpp/rclcpp.hpp>
#include <NvInfer.h>
#include <cuda_runtime.h>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_cpp/msg/object_detection.hpp"
#include "snappy_cpp/msg/bounding_box2_d.hpp"
#include "snappy_cpp/msg/polygon2_d.hpp"
#include <geometry_msgs/msg/point32.hpp>
#include "include/detection_classes.hpp"


using namespace std::chrono_literals;

std::vector<std::string> class_names_ = snappy_cpp::kFrontDetectionClasses;

class FrontCameraVision : public rclcpp::Node
{
public:
    explicit FrontCameraVision(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : Node("front_camera_vision", options)
    {
        // Declare parameters
        inference_hz_ = this->declare_parameter<double>("inference_hz", 10.0);
        conf_threshold_ = this->declare_parameter<double>("conf_threshold", 0.5);
        engine_path_ = this->declare_parameter<std::string>(
            "engine_path", "/home/kraken/Desktop/ffc_rs_26.engine");
        num_classes_ = this->declare_parameter<int>("num_classes", 8);

        RCLCPP_INFO(get_logger(), "Front Camera Vision (YOLOv26 Seg) initializing...");
        RCLCPP_INFO(get_logger(), "  inference_hz: %.1f", inference_hz_);
        RCLCPP_INFO(get_logger(), "  conf_threshold: %.2f", conf_threshold_);
        RCLCPP_INFO(get_logger(), "  engine_path: %s", engine_path_.c_str());

        init_tensorrt();

        // Subscribe to D455 color feed
        color_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/d455/color/image_raw",
            rclcpp::SensorDataQoS(),
            std::bind(&FrontCameraVision::color_callback, this, std::placeholders::_1));

        // Publish detections
        detection_pub_ = this->create_publisher<snappy_cpp::msg::DetectionArray>(
            "/d455/detections", 10);

        // Start inference worker
        worker_running_ = true;
        worker_thread_ = std::thread(&FrontCameraVision::inference_worker, this);

        RCLCPP_INFO(get_logger(), "Front Camera Vision ready");
    }

    ~FrontCameraVision() override
    {
        worker_running_ = false;
        queue_cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        if (context_) { delete context_; context_ = nullptr; }
        if (engine_) { delete engine_; engine_ = nullptr; }
        if (runtime_) { delete runtime_; runtime_ = nullptr; }

        if (device_input_) cudaFree(device_input_);
        if (device_output0_) cudaFree(device_output0_);
        if (device_output1_) cudaFree(device_output1_);
        if (host_output0_) cudaFreeHost(host_output0_);
        if (host_output1_) cudaFreeHost(host_output1_);
        if (stream_) cudaStreamDestroy(stream_);
    }

private:
    // ---- Data types --------------------------------------------------------

    struct InferenceJob {
        cv::Mat image;
        rclcpp::Time timestamp;
    };

    struct Detection {
        float x1, y1, x2, y2;
        float confidence;
        int class_id;
        std::vector<cv::Point2f> mask_polygon;
    };

    struct Anchor {
        float x, y, w, h;
        float confidence;
        int class_id;
        std::vector<float> mask_coeffs;
    };

    // ---- ROS callbacks -----------------------------------------------------

    void color_callback(const sensor_msgs::msg::Image::ConstSharedPtr & msg)
    {
        const auto now = this->get_clock()->now();
        if (last_inference_time_.has_value()) {
            const double elapsed = (now - *last_inference_time_).seconds();
            if (inference_hz_ > 0.0 && elapsed < (1.0 / inference_hz_)) {
                return;
            }
        }
        last_inference_time_ = now;

        try {
            auto cv_ptr = cv_bridge::toCvShare(msg, "bgr8");

            InferenceJob job;
            job.image = cv_ptr->image.clone();
            job.timestamp = rclcpp::Time(msg->header.stamp.sec, msg->header.stamp.nanosec);

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                // Drop oldest frame if queue is full (never accumulate latency)
                while (job_queue_.size() >= 2) {
                    job_queue_.pop_front();
                }
                job_queue_.push_back(std::move(job));
            }
            queue_cv_.notify_one();
        } catch (const cv_bridge::Exception & e) {
            RCLCPP_ERROR(get_logger(), "cv_bridge error: %s", e.what());
        }
    }

    // ---- Inference worker thread -------------------------------------------

    void inference_worker()
    {
        while (worker_running_) {
            InferenceJob job;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] {
                    return !worker_running_ || !job_queue_.empty();
                });
                if (!worker_running_ && job_queue_.empty()) break;
                job = std::move(job_queue_.front());
                job_queue_.pop_front();
            }

            if (job.image.empty()) continue;

            auto detections = run_inference(job.image);
            publish_detections(detections, job.timestamp);
        }
    }

    // ---- Core inference pipeline -------------------------------------------

    std::vector<Detection> run_inference(const cv::Mat & image)
    {
        auto t0 = std::chrono::steady_clock::now();

        const int orig_h = image.rows;
        const int orig_w = image.cols;

        // Letterbox resize
        cv::Mat resized;
        float scale;
        int pad_x, pad_y;
        letterbox(image, resized, scale, pad_x, pad_y);

        // BGR -> RGB, normalize to [0,1], HWC -> CHW
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
        resized.convertTo(resized, CV_32F, 1.0 / 255.0);

        std::vector<float> input_data(3 * input_h_ * input_w_);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < input_h_; ++h) {
                for (int w = 0; w < input_w_; ++w) {
                    input_data[c * input_h_ * input_w_ + h * input_w_ + w] =
                        resized.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        cudaMemcpyAsync(device_input_, input_data.data(),
                        input_data.size() * sizeof(float),
                        cudaMemcpyHostToDevice, stream_);

        context_->enqueueV3(stream_);

        cudaMemcpyAsync(host_output0_, device_output0_,
                        output0_size_ * sizeof(float),
                        cudaMemcpyDeviceToHost, stream_);
        cudaMemcpyAsync(host_output1_, device_output1_,
                        output1_size_ * sizeof(float),
                        cudaMemcpyDeviceToHost, stream_);

        cudaStreamSynchronize(stream_);

        auto detections = parse_segmentation_output(
            host_output0_, host_output1_, orig_w, orig_h, scale, pad_x, pad_y);

        auto t1 = std::chrono::steady_clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
        RCLCPP_INFO_THROTTLE(get_logger(), *this->get_clock(), 1000,
                             "Inference: %.2f ms (%.1f FPS), %zu detections",
                             ms, 1000.0f / ms, detections.size());

        return detections;
    }

    // ---- Preprocessing -----------------------------------------------------

    void letterbox(const cv::Mat & src, cv::Mat & dst,
                   float & scale, int & pad_x, int & pad_y)
    {
        scale = std::min(static_cast<float>(input_w_) / src.cols,
                         static_cast<float>(input_h_) / src.rows);
        int new_w = static_cast<int>(src.cols * scale);
        int new_h = static_cast<int>(src.rows * scale);
        pad_x = (input_w_ - new_w) / 2;
        pad_y = (input_h_ - new_h) / 2;

        cv::resize(src, dst, cv::Size(new_w, new_h));
        cv::copyMakeBorder(dst, dst, pad_y, pad_y, pad_x, pad_x,
                           cv::BORDER_CONSTANT, cv::Scalar(114, 114, 114));
    }

    // ---- YOLOv26 segmentation post-processing ------------------------------

    std::vector<Detection> parse_segmentation_output(
        const float * output0, const float * output1,
        int orig_w, int orig_h, float scale, int pad_x, int pad_y)
    {
        // YOLOv26 seg output0 layout: [1, (4 + num_classes + 32), 8400]
        // output1 (proto masks) layout: [1, 32, mask_h_, mask_w_]
        const int num_anchors = 8400;
        const int num_mask_coeffs = 32;
        const int values_per_anchor = 4 + num_classes_ + num_mask_coeffs;

        std::vector<Anchor> anchors;
        anchors.reserve(256);

        for (int i = 0; i < num_anchors; ++i) {
            const int offset = i * values_per_anchor;

            float max_conf = 0.0f;
            int best_class = 0;
            for (int c = 0; c < num_classes_; ++c) {
                float conf = output0[offset + 4 + c];
                if (conf > max_conf) {
                    max_conf = conf;
                    best_class = c;
                }
            }

            if (max_conf < static_cast<float>(conf_threshold_)) continue;

            Anchor a;
            a.x = output0[offset + 0];
            a.y = output0[offset + 1];
            a.w = output0[offset + 2];
            a.h = output0[offset + 3];
            a.confidence = max_conf;
            a.class_id = best_class;

            a.mask_coeffs.resize(num_mask_coeffs);
            for (int k = 0; k < num_mask_coeffs; ++k) {
                a.mask_coeffs[k] = output0[offset + 4 + num_classes_ + k];
            }
            anchors.push_back(a);
        }

        // Sort by confidence descending for NMS
        std::sort(anchors.begin(), anchors.end(),
                  [](const Anchor & a, const Anchor & b) {
                      return a.confidence > b.confidence;
                  });

        // Simple greedy NMS
        std::vector<bool> suppressed(anchors.size(), false);
        constexpr float iou_threshold = 0.45f;

        for (size_t i = 0; i < anchors.size(); ++i) {
            if (suppressed[i]) continue;
            for (size_t j = i + 1; j < anchors.size(); ++j) {
                if (suppressed[j]) continue;
                if (compute_iou(anchors[i], anchors[j]) > iou_threshold) {
                    suppressed[j] = true;
                }
            }
        }

        // Convert surviving anchors to original-image-space detections,
        // decoding the per-instance mask from the proto tensor along the way.
        std::vector<Detection> detections;
        constexpr float mask_threshold = 0.5f;

        for (size_t i = 0; i < anchors.size(); ++i) {
            if (suppressed[i]) continue;
            const auto & a = anchors[i];

            Detection det;
            det.x1 = std::clamp((a.x - a.w / 2.0f - pad_x) / scale, 0.0f, static_cast<float>(orig_w));
            det.y1 = std::clamp((a.y - a.h / 2.0f - pad_y) / scale, 0.0f, static_cast<float>(orig_h));
            det.x2 = std::clamp((a.x + a.w / 2.0f - pad_x) / scale, 0.0f, static_cast<float>(orig_w));
            det.y2 = std::clamp((a.y + a.h / 2.0f - pad_y) / scale, 0.0f, static_cast<float>(orig_h));
            det.confidence = a.confidence;
            det.class_id = a.class_id;
            det.mask_polygon = decode_mask_polygon(a.mask_coeffs, output1, det,
                                                    orig_w, orig_h, pad_x, pad_y,
                                                    mask_threshold);

            detections.push_back(std::move(det));
        }

        return detections;
    }

    // ---- Mask decoding: proto matmul -> sigmoid -> threshold -> contour ----

    std::vector<cv::Point2f> decode_mask_polygon(
        const std::vector<float> & mask_coeffs,
        const float * proto,
        const Detection & det,
        int orig_w, int orig_h,
        int pad_x, int pad_y,
        float mask_threshold)
    {
        const int num_mask_coeffs = static_cast<int>(mask_coeffs.size());

        // coeffs . proto -> low-res mask, then sigmoid
        cv::Mat mask_low(mask_h_, mask_w_, CV_32F, cv::Scalar(0.0f));
        for (int my = 0; my < mask_h_; ++my) {
            for (int mx = 0; mx < mask_w_; ++mx) {
                float sum = 0.0f;
                for (int k = 0; k < num_mask_coeffs; ++k) {
                    sum += mask_coeffs[k] * proto[k * mask_h_ * mask_w_ + my * mask_w_ + mx];
                }
                mask_low.at<float>(my, mx) = 1.0f / (1.0f + std::exp(-sum));
            }
        }

        // Upscale proto mask to letterboxed input size
        cv::Mat mask_input;
        cv::resize(mask_low, mask_input, cv::Size(input_w_, input_h_), 0, 0, cv::INTER_LINEAR);

        // Crop out the letterbox padding, leaving only the real-image region
        cv::Rect crop_roi(pad_x, pad_y, input_w_ - 2 * pad_x, input_h_ - 2 * pad_y);
        crop_roi &= cv::Rect(0, 0, mask_input.cols, mask_input.rows);
        if (crop_roi.width <= 0 || crop_roi.height <= 0) {
            return {};
        }
        cv::Mat mask_cropped = mask_input(crop_roi);

        // Resize to original image dimensions
        cv::Mat mask_full;
        cv::resize(mask_cropped, mask_full, cv::Size(orig_w, orig_h), 0, 0, cv::INTER_LINEAR);

        // Threshold to binary
        cv::Mat mask_bin;
        cv::threshold(mask_full, mask_bin, mask_threshold, 255, cv::THRESH_BINARY);
        mask_bin.convertTo(mask_bin, CV_8U);

        // Gate by the detection's own box (proto masks are class-agnostic;
        // the box tells us which instance this mask belongs to)
        cv::Rect box(static_cast<int>(det.x1), static_cast<int>(det.y1),
                    static_cast<int>(det.x2 - det.x1), static_cast<int>(det.y2 - det.y1));
        box &= cv::Rect(0, 0, orig_w, orig_h);

        cv::Mat box_mask = cv::Mat::zeros(mask_bin.size(), CV_8U);
        if (box.width > 0 && box.height > 0) {
            mask_bin(box).copyTo(box_mask(box));
        }

        // Extract the largest external contour as the polygon
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(box_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        if (contours.empty()) {
            return {};
        }

        auto largest = std::max_element(contours.begin(), contours.end(),
            [](const auto & a, const auto & b) { return a.size() < b.size(); });

        std::vector<cv::Point2f> polygon;
        polygon.reserve(largest->size());
        for (const auto & pt : *largest) {
            polygon.emplace_back(static_cast<float>(pt.x), static_cast<float>(pt.y));
        }
        return polygon;
    }

    static float compute_iou(const Anchor & a, const Anchor & b)
    {
        float x1 = std::max(a.x - a.w / 2.0f, b.x - b.w / 2.0f);
        float y1 = std::max(a.y - a.h / 2.0f, b.y - b.h / 2.0f);
        float x2 = std::min(a.x + a.w / 2.0f, b.x + b.w / 2.0f);
        float y2 = std::min(a.y + a.h / 2.0f, b.y + b.h / 2.0f);

        float inter = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
        float union_area = a.w * a.h + b.w * b.h - inter;
        return (union_area > 0.0f) ? (inter / union_area) : 0.0f;
    }

    // ---- Publishing --------------------------------------------------------

    void publish_detections(const std::vector<Detection> & detections,
                            const rclcpp::Time & timestamp)
    {
        auto msg = snappy_cpp::msg::DetectionArray();
        msg.header.stamp = timestamp;
        msg.header.frame_id = "d455_color_optical_frame";
        msg.detections.reserve(detections.size());

        for (const auto & det : detections) {
            snappy_cpp::msg::ObjectDetection obj;
            obj.confidence = det.confidence;
            obj.object_class = class_name_for(det.class_id);
            obj.distance_m = -1.0f;  // not computed in this node
            obj.timestamp = timestamp;

            snappy_cpp::msg::BoundingBox2D bbox;
            bbox.x = det.x1;
            bbox.y = det.y1;
            bbox.width = det.x2 - det.x1;
            bbox.height = det.y2 - det.y1;
            obj.bounding_box = bbox;

            snappy_cpp::msg::Polygon2D poly;
            poly.points.reserve(det.mask_polygon.size());
            for (const auto & pt : det.mask_polygon) {
                geometry_msgs::msg::Point32 p;
                p.x = pt.x;
                p.y = pt.y;
                p.z = 0.0f;
                poly.points.push_back(p);
            }
            obj.mask_polygons.push_back(std::move(poly));

            msg.detections.push_back(std::move(obj));
        }

        detection_pub_->publish(msg);
    }

    // ---- Class label lookup -------------------------------------------------

    std::string class_name_for(int class_id) const
    {
        if (class_id >= 0 && static_cast<size_t>(class_id) < class_names_.size()) {
            return class_names_[class_id];
        }
        return "unknown";
    }

    // ---- TensorRT initialization -------------------------------------------

    void init_tensorrt()
    {
        std::ifstream file(engine_path_, std::ios::binary);
        if (!file.good()) {
            throw std::runtime_error("Failed to open TensorRT engine: " + engine_path_);
        }

        file.seekg(0, std::ios::end);
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        std::vector<char> engine_data(size);
        file.read(engine_data.data(), static_cast<std::streamsize>(size));
        file.close();

        runtime_ = nvinfer1::createInferRuntime(trt_logger_);
        if (!runtime_) throw std::runtime_error("Failed to create TRT runtime");

        engine_ = runtime_->deserializeCudaEngine(engine_data.data(), size);
        if (!engine_) throw std::runtime_error("Failed to deserialize TRT engine");

        context_ = engine_->createExecutionContext();
        if (!context_) throw std::runtime_error("Failed to create TRT execution context");

        // YOLOv26 seg has 3 I/O tensors: input, output0 (boxes+coeffs), output1 (proto)
        input_name_ = engine_->getIOTensorName(0);
        output0_name_ = engine_->getIOTensorName(1);
        output1_name_ = engine_->getIOTensorName(2);

        auto input_dims = engine_->getTensorShape(input_name_.c_str());
        input_h_ = input_dims.d[2];
        input_w_ = input_dims.d[3];

        auto out0_dims = engine_->getTensorShape(output0_name_.c_str());
        output0_size_ = 1;
        for (int i = 0; i < out0_dims.nbDims; ++i) output0_size_ *= out0_dims.d[i];

        auto out1_dims = engine_->getTensorShape(output1_name_.c_str());
        output1_size_ = 1;
        for (int i = 0; i < out1_dims.nbDims; ++i) output1_size_ *= out1_dims.d[i];
        mask_h_ = out1_dims.d[2];
        mask_w_ = out1_dims.d[3];

        // Allocate device buffers
        cudaStreamCreate(&stream_);
        cudaMalloc(&device_input_,   3 * input_h_ * input_w_ * sizeof(float));
        cudaMalloc(&device_output0_, output0_size_ * sizeof(float));
        cudaMalloc(&device_output1_, output1_size_ * sizeof(float));

        // Pinned host buffers for async D2H
        cudaHostAlloc(&host_output0_, output0_size_ * sizeof(float), cudaHostAllocDefault);
        cudaHostAlloc(&host_output1_, output1_size_ * sizeof(float), cudaHostAllocDefault);

        // Bind tensor addresses once
        context_->setTensorAddress(input_name_.c_str(),   device_input_);
        context_->setTensorAddress(output0_name_.c_str(), device_output0_);
        context_->setTensorAddress(output1_name_.c_str(), device_output1_);

        RCLCPP_INFO(get_logger(), "Engine loaded: %dx%d input | out0=%zu | out1=%zu",
                     input_w_, input_h_, output0_size_, output1_size_);
    }

    // ---- TRT logger --------------------------------------------------------

    class TRTLogger : public nvinfer1::ILogger {
        void log(Severity severity, const char * msg) noexcept override {
            if (severity <= Severity::kWARNING)
                std::cerr << "[TRT] " << msg << std::endl;
        }
    } trt_logger_;

    // ---- Members -----------------------------------------------------------

    // ROS
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr color_sub_;
    rclcpp::Publisher<snappy_cpp::msg::DetectionArray>::SharedPtr detection_pub_;

    // TensorRT / CUDA
    nvinfer1::IRuntime *         runtime_      = nullptr;
    nvinfer1::ICudaEngine *      engine_       = nullptr;
    nvinfer1::IExecutionContext * context_     = nullptr;
    cudaStream_t                 stream_       = nullptr;
    float * device_input_   = nullptr;
    float * device_output0_ = nullptr;
    float * device_output1_ = nullptr;
    float * host_output0_   = nullptr;
    float * host_output1_   = nullptr;

    std::string input_name_, output0_name_, output1_name_;
    int    input_w_ = 640, input_h_ = 640;
    size_t output0_size_ = 0, output1_size_ = 0;

    // Threading
    std::deque<InferenceJob>   job_queue_;
    std::mutex                 queue_mutex_;
    std::condition_variable    queue_cv_;
    std::thread                worker_thread_;
    std::atomic<bool>          worker_running_{false};

    // Parameters
    double      inference_hz_    = 10.0;
    double      conf_threshold_  = 0.5;
    int         num_classes_     = 80;
    std::string engine_path_;
    std::optional<rclcpp::Time> last_inference_time_;

    int mask_h_ = 0, mask_w_ = 0;

    // TODO: fill in with your own class labels, indexed by class_id.
    // Must have num_classes_ entries (or class_name_for() will return "unknown"
    // for any id past the end of this vector).
    std::vector<std::string> class_names_ = {
        // "class_0_name", "class_1_name", ...
    };
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<FrontCameraVision>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
