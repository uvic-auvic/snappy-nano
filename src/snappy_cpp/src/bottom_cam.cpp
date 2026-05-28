#include <rclcpp/qos.hpp>
#include <rclcpp/rclcpp.hpp>
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime.h>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_cpp/msg/object_detection.hpp"
#include "snappy_cpp/msg/bounding_box2_d.hpp"

using namespace std::chrono_literals;

// COCO class names (80 classes)
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

class BottomCamNode: public rclcpp::Node
{
public:
    BottomCamNode() : Node("bottom_cam")
    {
        RCLCPP_INFO(get_logger(), "=== CUDA + TensorRT YOLO Node ===");

        // Declare and get camera namespace parameter
        this->declare_parameter<std::string>("camera_namespace", "camera");
        this->declare_parameter<double>("inference_hz", 10.0);
        this->declare_parameter<bool>("display", false);
        this->declare_parameter<int>("distance_samples", 100);

        camera_namespace_ = this->get_parameter("camera_namespace").as_string();
        inference_hz_ = this->get_parameter("inference_hz").as_double();
        display_ = this->get_parameter("display").as_bool();
        distance_samples_ = std::max(1, this->get_parameter("distance_samples").as_int());

        RCLCPP_INFO(get_logger(), "CUDA Node initialized for camera: %s", camera_namespace_.c_str());
        RCLCPP_INFO(get_logger(), "Settings: inference_hz=%.1f, display=%s, distance_samples=%d",
                    inference_hz_, display_ ? "true" : "false", distance_samples_);

        checkCUDA();
        build_engine_from_onnx();
        allocate_buffers();

        display_window_name_ = "Detections (TensorRT) [" + camera_namespace_ + "]";
        if (display_) {
            cv::namedWindow(display_window_name_, cv::WINDOW_AUTOSIZE);
            RCLCPP_INFO(get_logger(), "OpenCV window created: %s", display_window_name_.c_str());
        }

        // Create publisher for detections with camera-specific topic
        std::string detection_topic = "/" + camera_namespace_ + "/detections";
        detection_publisher_ = this->create_publisher<snappy_cpp::msg::DetectionArray>(
            detection_topic, rclcpp::SensorDataQoS());
        RCLCPP_INFO(get_logger(), "Created detection publisher on %s", detection_topic.c_str());

        // Build topic names using camera namespace
        std::string color_topic = "/" + camera_namespace_ + "/" + camera_namespace_ + "/color/image_raw";
        std::string depth_topic = "/" + camera_namespace_ + "/" + camera_namespace_ + "/aligned_depth_to_color/image_raw";

        color_sub_.subscribe(this, color_topic);
        depth_sub_.subscribe(this, depth_topic);
        sync_ = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(SyncPolicy(10), color_sub_, depth_sub_);
        sync_->registerCallback(std::bind(&BottomCamNode::sync_callback, this, std::placeholders::_1, std::placeholders::_2));

        RCLCPP_INFO(get_logger(), "Subscribed to color: %s", color_topic.c_str());
        RCLCPP_INFO(get_logger(), "Subscribed to depth: %s", depth_topic.c_str());
    }

    ~BottomCamNode() override
    {
        if (context_ != nullptr) {
            delete context_;
            context_ = nullptr;
        }
        if (engine_ != nullptr) {
            delete engine_;
            engine_ = nullptr;
        }
        if (device_buffers_[0] != nullptr) {
            cudaFree(device_buffers_[0]);
            device_buffers_[0] = nullptr;
        }
        if (device_buffers_[1] != nullptr) {
            cudaFree(device_buffers_[1]);
            device_buffers_[1] = nullptr;
        }
        if (stream_ != nullptr) {
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
        if (display_) {
            cv::destroyWindow(display_window_name_);
        }
    }


private:
    // Detection struct matching computer_vision.cpp
    struct Detection {
        cv::Rect2f box;   // x, y, width, height in original image pixels
        float conf;
        int   class_id;
        float distance_m = -1.0f;
    };

    // Detection results with inference time and timestamp
    struct DetectionResult {
        std::vector<Detection> detections;
        float inference_time_ms;
        rclcpp::Time timestamp;
    };

    // Constants matching computer_vision.cpp
    static constexpr float CONF_THRESH = 0.25f;  // lowered for sensitivity - NMS-free head
    static constexpr int NUM_CLASSES = 80;

    // Message filter synchronization policy
    typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::msg::Image, sensor_msgs::msg::Image> SyncPolicy;

    class TRTLogger : public nvinfer1::ILogger {
    public:
        void log(Severity severity, const char* msg) noexcept override {
            if (severity <= Severity::kWARNING) {
                std::cerr << "[TensorRT] " << msg << std::endl;
            }
        }
    };

    void checkCUDA() {
        int deviceCount = 0;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess) {
            RCLCPP_ERROR(this->get_logger(), "Failed to get device count: %s", cudaGetErrorString(err));
            throw std::runtime_error("CUDA unavailable");
        }

        RCLCPP_INFO(this->get_logger(), "Found %d CUDA devices", deviceCount);

        if (deviceCount == 0) {
            throw std::runtime_error("No CUDA devices found");
        }
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        RCLCPP_INFO(get_logger(), "GPU: %s", prop.name);
        RCLCPP_INFO(get_logger(), "Compute capability: %d.%d", prop.major, prop.minor);
        RCLCPP_INFO(get_logger(), "GPU Memory: %zu MB", prop.totalGlobalMem / (1024 * 1024));
        RCLCPP_INFO(get_logger(), "CUDA: OK ✅");
    }

    static size_t volume(const nvinfer1::Dims& dims)
    {
        size_t result = 1;
        for (int i = 0; i < dims.nbDims; ++i) {
            result *= static_cast<size_t>(dims.d[i]);
        }
        return result;
    }

    std::string resolve_model_path(const std::string& extension)
    {
        std::string model_path =
            ament_index_cpp::get_package_share_directory("snappy_cpp") + "/models/yolo26s." + extension;

        if (!std::filesystem::exists(model_path)) {
            throw std::runtime_error("Model not found at " + model_path);
        }
        return model_path;
    }

    void build_engine_from_onnx()
    {
        // Try to load pre-built .engine file first (instant startup)
        std::string engine_path;
        std::string cache_dir = std::string(getenv("HOME")) + "/.cache/snappy_cpp";
        std::string cached_engine_path = cache_dir + "/yolo26s.engine";
        bool has_engine = false;

        // Check for engine in install directory first
        try {
            engine_path = resolve_model_path("engine");
            has_engine = true;
            RCLCPP_INFO(get_logger(), "Found pre-built TensorRT engine: %s", engine_path.c_str());
        } catch (...) {
            // Check for cached engine
            if (std::filesystem::exists(cached_engine_path)) {
                engine_path = cached_engine_path;
                has_engine = true;
                RCLCPP_INFO(get_logger(), "Found cached TensorRT engine: %s", engine_path.c_str());
            } else {
                RCLCPP_WARN(get_logger(), "No pre-built .engine file found, will build from ONNX");
            }
        }

        auto runtime = std::unique_ptr<nvinfer1::IRuntime, TRTDestroyer>(
            nvinfer1::createInferRuntime(trt_logger_));
        if (!runtime) {
            throw std::runtime_error("Failed to create TensorRT runtime");
        }

        if (has_engine) {
            // Load pre-built engine (fast path)
            std::ifstream engine_file(engine_path, std::ios::binary);
            if (!engine_file) {
                throw std::runtime_error("Failed to open engine file: " + engine_path);
            }

            engine_file.seekg(0, std::ios::end);
            size_t engine_size = engine_file.tellg();
            engine_file.seekg(0, std::ios::beg);

            std::vector<char> engine_data(engine_size);
            engine_file.read(engine_data.data(), engine_size);
            engine_file.close();

            engine_ = runtime->deserializeCudaEngine(engine_data.data(), engine_size);
            if (!engine_) {
                throw std::runtime_error("Failed to deserialize pre-built TensorRT engine");
            }
            RCLCPP_INFO(get_logger(), "✅ Loaded pre-built TensorRT engine (instant startup!)");
        } else {
            // Build from ONNX (slow path - first time only)
            const std::string model_path = resolve_model_path("onnx");
            RCLCPP_INFO(get_logger(), "Building TensorRT engine from ONNX: %s", model_path.c_str());
            RCLCPP_INFO(get_logger(), "⏳ This may take 2-5 minutes on first run...");

            auto builder = std::unique_ptr<nvinfer1::IBuilder, TRTDestroyer>(
                nvinfer1::createInferBuilder(trt_logger_));
            if (!builder) {
                throw std::runtime_error("Failed to create TensorRT builder");
            }

            const uint32_t flags = 1U << static_cast<uint32_t>(
                nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
            auto network = std::unique_ptr<nvinfer1::INetworkDefinition, TRTDestroyer>(
                builder->createNetworkV2(flags));
            if (!network) {
                throw std::runtime_error("Failed to create TensorRT network");
            }

            auto parser = std::unique_ptr<nvonnxparser::IParser, TRTDestroyer>(
                nvonnxparser::createParser(*network, trt_logger_));
            if (!parser) {
                throw std::runtime_error("Failed to create TensorRT ONNX parser");
            }

            if (!parser->parseFromFile(model_path.c_str(), static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
                throw std::runtime_error("TensorRT failed to parse ONNX model");
            }

            auto config = std::unique_ptr<nvinfer1::IBuilderConfig, TRTDestroyer>(
                builder->createBuilderConfig());
            if (!config) {
                throw std::runtime_error("Failed to create TensorRT builder config");
            }

            config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, 1ULL << 30);

            if (builder->platformHasFastFp16()) {
                config->setFlag(nvinfer1::BuilderFlag::kFP16);
                RCLCPP_INFO(get_logger(), "TensorRT FP16 enabled");
            }

            // TensorRT 10 API: serialize the network and then deserialize with runtime
            auto serialized_network = std::unique_ptr<nvinfer1::IHostMemory, TRTDestroyer>(
                builder->buildSerializedNetwork(*network, *config));
            if (!serialized_network) {
                throw std::runtime_error("Failed to serialize TensorRT network");
            }

            // Save the serialized engine to cache for future runs
            std::filesystem::create_directories(cache_dir);
            std::ofstream cache_file(cached_engine_path, std::ios::binary);
            if (cache_file) {
                cache_file.write(static_cast<const char*>(serialized_network->data()), serialized_network->size());
                cache_file.close();
                RCLCPP_INFO(get_logger(), "💾 Cached TensorRT engine to: %s", cached_engine_path.c_str());
            } else {
                RCLCPP_WARN(get_logger(), "Failed to cache engine (non-fatal)");
            }

            engine_ = runtime->deserializeCudaEngine(serialized_network->data(), serialized_network->size());
            if (!engine_) {
                throw std::runtime_error("Failed to deserialize TensorRT engine");
            }
            RCLCPP_INFO(get_logger(), "✅ TensorRT engine built successfully!");
        }

        context_ = engine_->createExecutionContext();
        if (!context_) {
            throw std::runtime_error("Failed to create TensorRT execution context");
        }

        // TensorRT 10 API: use getNbIOTensors() instead of getNbBindings()
        int32_t nb_tensors = engine_->getNbIOTensors();
        for (int32_t i = 0; i < nb_tensors; ++i) {
            const char* tensor_name = engine_->getIOTensorName(i);
            nvinfer1::TensorIOMode io_mode = engine_->getTensorIOMode(tensor_name);

            if (io_mode == nvinfer1::TensorIOMode::kINPUT) {
                input_name_ = tensor_name;
                input_index_ = i;
            } else if (io_mode == nvinfer1::TensorIOMode::kOUTPUT) {
                output_name_ = tensor_name;
                output_index_ = i;
            }
        }

        if (input_index_ < 0 || output_index_ < 0) {
            throw std::runtime_error("Could not resolve TensorRT input/output tensors");
        }

        // TensorRT 10 API: use getTensorShape() instead of getBindingDimensions()
        nvinfer1::Dims input_dims = engine_->getTensorShape(input_name_.c_str());
        if (input_dims.nbDims == 4 && input_dims.d[0] == -1) {
            input_dims.d[0] = 1;
        }
        if (input_dims.nbDims == 4 && input_dims.d[2] > 0 && input_dims.d[3] > 0) {
            input_h_ = input_dims.d[2];
            input_w_ = input_dims.d[3];
        }

        // TensorRT 10 API: use setInputShape() instead of setBindingDimensions()
        if (!context_->setInputShape(input_name_.c_str(), input_dims)) {
            throw std::runtime_error("Failed to set TensorRT input shape");
        }

        // Get final input and output dimensions
        nvinfer1::Dims final_in_dims = context_->getTensorShape(input_name_.c_str());
        nvinfer1::Dims final_out_dims = context_->getTensorShape(output_name_.c_str());
        input_count_ = volume(final_in_dims);
        output_count_ = volume(final_out_dims);

        RCLCPP_INFO(get_logger(), "TensorRT ready. Input: %dx%d, output elements: %zu",
                    input_w_, input_h_, output_count_);
    }

    void allocate_buffers()
    {
        if (cudaStreamCreate(&stream_) != cudaSuccess) {
            throw std::runtime_error("Failed to create CUDA stream");
        }

        if (cudaMalloc(&device_buffers_[0], input_count_ * sizeof(float)) != cudaSuccess) {
            throw std::runtime_error("Failed to allocate input CUDA buffer");
        }
        if (cudaMalloc(&device_buffers_[1], output_count_ * sizeof(float)) != cudaSuccess) {
            throw std::runtime_error("Failed to allocate output CUDA buffer");
        }

        host_output_.resize(output_count_);
        input_tensor_.resize(input_count_);
    }

    // Letterbox preprocessing matching computer_vision.cpp
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

    void preprocess(const cv::Mat& image, float& scale, int& pad_x, int& pad_y)
    {
        // Apply letterbox to maintain aspect ratio
        cv::Mat letterboxed = letterboxFeed(image, input_w_, input_h_, scale, pad_x, pad_y);

        // BGR -> RGB conversion
        cv::Mat rgb;
        cv::cvtColor(letterboxed, rgb, cv::COLOR_BGR2RGB);

        // Normalize to [0, 1]
        cv::Mat float_img;
        rgb.convertTo(float_img, CV_32FC3, 1.0f / 255.0f);

        // Convert to NCHW format
        std::vector<cv::Mat> channels(3);
        cv::split(float_img, channels);

        const size_t plane = static_cast<size_t>(input_h_ * input_w_);
        for (int c = 0; c < 3; ++c) {
            std::memcpy(input_tensor_.data() + c * plane,
                        channels[c].data,
                        plane * sizeof(float));
        }
    }

    // Parse YOLO detections matching computer_vision.cpp
    std::vector<Detection> parse_detections(const std::vector<float>& output_data,
        int originalWidth, int originalHeight, float scale, int paddingX, int paddingY)
    {
        std::vector<Detection> result;
        result.reserve(300);

        // YOLO26n output format: (1, 300, 6) - 300 detections, each with 6 values
        // [x1, y1, x2, y2, confidence, class_id]
        const int num_detections = 300;
        const int values_per_detection = 6;

        for (int i = 0; i < num_detections; ++i) {
            const int offset = i * values_per_detection;

            float conf = output_data[offset + 4];
            if (conf < CONF_THRESH) continue;

            int class_id = static_cast<int>(output_data[offset + 5]);
            if (class_id < 0 || class_id >= NUM_CLASSES) continue;

            // YOLO26n default export: [x1, y1, x2, y2] already in corner format,
            // in letterboxed 640x640 pixel space
            float x1 = output_data[offset + 0];
            float y1 = output_data[offset + 1];
            float x2 = output_data[offset + 2];
            float y2 = output_data[offset + 3];

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

            // RCLCPP_INFO(this->get_logger(),
            //     "✓ Detection: %s | conf=%.2f | box=[%.0f, %.0f, %.0f, %.0f]",
            //     CLASS_NAMES[class_id].c_str(), conf, x1, y1, bw, bh);

            result.push_back({{x1, y1, bw, bh}, conf, class_id, -1.0f});
        }

        if (!result.empty()) {
            RCLCPP_DEBUG_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                "Detections this frame: %zu", result.size());
        }

        return result;
    }

    // this ill be called right after line 433 inside parse_detections funciton
    void data_for_missionPlanner(const std::vector<Detection>& [[maybe_unused]] detections) {


    }

    float distance_away(const Detection& d, const cv::Mat& depth_image) {
        if (depth_image.empty() || depth_image.type() != CV_16UC1) {
            return -1.0f;
        }

        int x1 = std::clamp(static_cast<int>(d.box.x), 0, depth_image.cols - 1);
        int y1 = std::clamp(static_cast<int>(d.box.y), 0, depth_image.rows - 1);
        int x2 = std::clamp(static_cast<int>(d.box.x + d.box.width), 0, depth_image.cols);
        int y2 = std::clamp(static_cast<int>(d.box.y + d.box.height), 0, depth_image.rows);

        if (x2 <= x1 || y2 <= y1) {
            return -1.0f;
        }

        const int roi_w = x2 - x1;
        const int roi_h = y2 - y1;
        const int grid = std::max(1, static_cast<int>(std::sqrt(static_cast<double>(distance_samples_))));
        const int step_x = std::max(1, roi_w / grid);
        const int step_y = std::max(1, roi_h / grid);

        std::vector<uint16_t> depth_values;
        depth_values.reserve(static_cast<size_t>(grid * grid));

        const int y_offset = step_y / 2;
        const int x_offset = step_x / 2;
        for (int y = y1 + y_offset; y < y2; y += step_y) {
            const uint16_t* row_ptr = depth_image.ptr<uint16_t>(y);
            for (int x = x1 + x_offset; x < x2; x += step_x) {
                uint16_t depth_mm = row_ptr[x];
                if (depth_mm > 0) {
                    depth_values.push_back(depth_mm);
                }
            }
        }

        if (depth_values.empty()) {
            return -1.0f;
        }

        auto mid = depth_values.begin() + (depth_values.size() / 2);
        std::nth_element(depth_values.begin(), mid, depth_values.end());
        return static_cast<float>(*mid) / 1000.0f;
    }

    // Draw detections matching computer_vision.cpp
    void draw_detections(cv::Mat& frame, const std::vector<Detection>& detections)
    {
        for (const auto& d : detections) {
            int x = (int)d.box.x;
            int y = (int)d.box.y;
            int w = (int)d.box.width;
            int h = (int)d.box.height;

            cv::Rect rect(x, y, w, h);
            cv::rectangle(frame, rect, cv::Scalar(0, 255, 0), 2);

            std::string distance_str = (d.distance_m > 0) ?
                std::to_string((int)(d.distance_m * 100)) + "cm" : "N/A";

            std::string label = CLASS_NAMES[d.class_id] + " " +
                                std::to_string((int)(d.conf * 100)) + "%" +
                                " [" + distance_str + "]";

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

    void run_inference(const cv::Mat& frame, rclcpp::Time timestamp, DetectionResult& result)
    {
        auto t0 = std::chrono::steady_clock::now();

        // Store preprocessing parameters
        float scale;
        int pad_x, pad_y;

        // Preprocess with letterboxing
        preprocess(frame, scale, pad_x, pad_y);

        // Copy input to GPU
        cudaMemcpyAsync(device_buffers_[0], input_tensor_.data(),
                        input_count_ * sizeof(float), cudaMemcpyHostToDevice, stream_);

        // Set input and output tensor addresses for TensorRT 10 API
        context_->setTensorAddress(input_name_.c_str(), device_buffers_[0]);
        context_->setTensorAddress(output_name_.c_str(), device_buffers_[1]);

        // TensorRT 10 API: use enqueueV3() with CUDA stream only
        if (!context_->enqueueV3(stream_)) {
            RCLCPP_ERROR(get_logger(), "TensorRT enqueue failed");
            return;
        }

        // Copy output from GPU
        cudaMemcpyAsync(host_output_.data(), device_buffers_[1],
                        output_count_ * sizeof(float), cudaMemcpyDeviceToHost, stream_);
        cudaStreamSynchronize(stream_);

        auto t1 = std::chrono::steady_clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

        // Parse detections
        std::vector<Detection> detections = parse_detections(
            host_output_, frame.cols, frame.rows, scale, pad_x, pad_y);

        // Store results
        result.detections = detections;
        result.inference_time_ms = ms;
        result.timestamp = timestamp;

    }

    void sync_callback(const sensor_msgs::msg::Image::ConstSharedPtr color_msg, const sensor_msgs::msg::Image::ConstSharedPtr depth_msg) {
        try {
            if (!color_msg || !depth_msg) {
                RCLCPP_ERROR(this->get_logger(), "Received null image message");
                return;
            }

            // Convert color image
            cv_bridge::CvImageConstPtr color_ptr = cv_bridge::toCvShare(color_msg, "bgr8");
            cv::Mat color_image = color_ptr->image;

            if (color_image.empty()) {
                RCLCPP_ERROR(this->get_logger(), "Received empty color image from cv_bridge");
                return;
            }

            // Convert depth image (16-bit grayscale, 1 channel)
            cv_bridge::CvImageConstPtr depth_ptr = cv_bridge::toCvShare(depth_msg, "16UC1");
            cv::Mat depth_image = depth_ptr->image;

            if (depth_image.empty()) {
                RCLCPP_ERROR(this->get_logger(), "Received empty depth image from cv_bridge");
                return;
            }

            const rclcpp::Time now = this->get_clock()->now();
            const double min_inference_period = inference_hz_ > 0.0 ? (1.0 / inference_hz_) : 0.0;
            if (min_inference_period > 0.0 && last_inference_time_.has_value() &&
                (now - *last_inference_time_).seconds() < min_inference_period) {
                return;
            }
            last_inference_time_ = now;

            // Run TensorRT inference with depth data
            DetectionResult det_result;
            rclcpp::Time timestamp(color_msg->header.stamp.sec, color_msg->header.stamp.nanosec);
            run_inference(color_image, timestamp, det_result);

            for (auto& detection : det_result.detections) {
                detection.distance_m = distance_away(detection, depth_image);
            }

            // Publish detections
            publish_detections(det_result);

            if (display_) {
                cv::Mat annotated = color_image.clone();
                draw_detections(annotated, det_result.detections);

                std::string info = "Infer: " + std::to_string((int)det_result.inference_time_ms) + "ms (TensorRT GPU)"
                                 + "  det: " + std::to_string(det_result.detections.size());
                cv::putText(annotated, info, {10, 30},
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

                cv::imshow(display_window_name_, annotated);

                int x_pos = 0;
                if (camera_namespace_.find("455") != std::string::npos) {
                    x_pos = 680;
                }
                cv::moveWindow(display_window_name_, x_pos, 0);

                int key = cv::waitKey(1);
                if (key == 27) {
                    RCLCPP_INFO(this->get_logger(), "ESC key pressed, exiting");
                    rclcpp::shutdown();
                }
            }
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Inference exception: %s", e.what());
        }
    }

    void publish_detections(const DetectionResult& det_result)
    {
        auto detection_array = std::make_unique<snappy_cpp::msg::DetectionArray>();
        detection_array->header.stamp = det_result.timestamp;
        detection_array->header.frame_id = "camera_link";
        detection_array->inference_time_ms = static_cast<uint32_t>(det_result.inference_time_ms);

        for (const auto& detection : det_result.detections) {
            snappy_cpp::msg::ObjectDetection obj_det;
            obj_det.object_class = CLASS_NAMES[detection.class_id];
            obj_det.confidence = detection.conf;

            obj_det.distance_m = detection.distance_m;

            // Populate bounding box
            obj_det.bounding_box.x = detection.box.x;
            obj_det.bounding_box.y = detection.box.y;
            obj_det.bounding_box.width = detection.box.width;
            obj_det.bounding_box.height = detection.box.height;

            // Convert rclcpp::Time to builtin_interfaces/Time
            obj_det.timestamp.sec = det_result.timestamp.seconds();
            obj_det.timestamp.nanosec = det_result.timestamp.nanoseconds() % 1000000000;

            detection_array->detections.push_back(obj_det);
        }

        detection_publisher_->publish(*detection_array);

        if (!detection_array->detections.empty()) {
            RCLCPP_DEBUG_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                "Published %zu detections (inference: %.1fms)",
                detection_array->detections.size(),
                det_result.inference_time_ms);
        }
    }

    struct TRTDestroyer {
        template <typename T>
        void operator()(T* object) const
        {
            if (object != nullptr) {
                delete object;
            }
        }
    };

    // Camera configuration
    std::string camera_namespace_;
    std::string display_window_name_;
    double inference_hz_ = 10.0;
    bool display_ = false;
    int distance_samples_ = 100;
    std::optional<rclcpp::Time> last_inference_time_;

    // Message filter subscriptions and synchronizer
    message_filters::Subscriber<sensor_msgs::msg::Image> color_sub_;
    message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub_;
    std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync_;

    // Detection publisher
    rclcpp::Publisher<snappy_cpp::msg::DetectionArray>::SharedPtr detection_publisher_;

    TRTLogger trt_logger_;

    nvinfer1::ICudaEngine* engine_ = nullptr;
    nvinfer1::IExecutionContext* context_ = nullptr;

    void* device_buffers_[2] = {nullptr, nullptr};
    cudaStream_t stream_ = nullptr;

    std::string input_name_;
    std::string output_name_;
    int32_t input_index_ = -1;
    int32_t output_index_ = -1;
    int input_w_ = 640;
    int input_h_ = 640;
    size_t input_count_ = 0;
    size_t output_count_ = 0;
    std::vector<float> host_output_;
    std::vector<float> input_tensor_;
};


int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<BottomCamNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
