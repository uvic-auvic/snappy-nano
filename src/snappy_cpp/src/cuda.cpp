#include <rclcpp/qos.hpp>
#include <rclcpp/rclcpp.hpp>
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime.h>
#include <ament_index_cpp/get_package_share_directory.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <mutex>

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

class CudaNode : public rclcpp::Node
{
public:
    CudaNode() : Node("cuda_node")
    {
        RCLCPP_INFO(get_logger(), "=== CUDA + TensorRT YOLO Node ===");
        checkCUDA();
        build_engine_from_onnx();
        allocate_buffers();

        cv::namedWindow("Detections (TensorRT)", cv::WINDOW_AUTOSIZE);
        RCLCPP_INFO(get_logger(), "OpenCV window created");

        colorCameraFeed_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/color/image_raw",
            rclcpp::SensorDataQoS(),
            std::bind(&CudaNode::color_callback, this, std::placeholders::_1)
        );
        RCLCPP_INFO(get_logger(), "Subscribed to /camera/camera/color/image_raw");
    }

    ~CudaNode() override
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
    }


private:
    // Detection struct matching computer_vision.cpp
    struct Detection {
        cv::Rect2f box;   // x, y, width, height in original image pixels
        float conf;
        int   class_id;
    };

    // Constants matching computer_vision.cpp
    static constexpr float CONF_THRESH = 0.25f;  // lowered for sensitivity - NMS-free head
    static constexpr int NUM_CLASSES = 80;

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

    std::vector<float> preprocess(const cv::Mat& image, float& scale, int& pad_x, int& pad_y)
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

        std::vector<float> input_tensor(input_count_);
        const size_t plane = static_cast<size_t>(input_h_ * input_w_);
        for (int c = 0; c < 3; ++c) {
            std::memcpy(input_tensor.data() + c * plane,
                        channels[c].data,
                        plane * sizeof(float));
        }

        return input_tensor;
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

    void run_inference(const cv::Mat& frame)
    {
        auto t0 = std::chrono::steady_clock::now();

        // Store preprocessing parameters
        float scale;
        int pad_x, pad_y;

        // Preprocess with letterboxing
        std::vector<float> input_tensor = preprocess(frame, scale, pad_x, pad_y);

        // Copy input to GPU
        cudaMemcpyAsync(device_buffers_[0], input_tensor.data(),
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

        // Draw detections on frame
        cv::Mat annotated = frame.clone();
        draw_detections(annotated, detections);

        // Add inference time and detection count info
        std::string info = "Infer: " + std::to_string((int)ms) + "ms (TensorRT GPU)"
                         + "  det: " + std::to_string(detections.size());
        cv::putText(annotated, info, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        // Display result
        {
            std::lock_guard<std::mutex> lock(display_mutex_);
            latest_detection_ = annotated;
        }
    }

    void color_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            if (!msg) {
                RCLCPP_ERROR(this->get_logger(), "Received null image message");
                return;
            }

            cv_bridge::CvImageConstPtr cv_ptr = cv_bridge::toCvShare(msg, "bgr8");
            cv::Mat color_image = cv_ptr->image;

            if (color_image.empty()) {
                RCLCPP_ERROR(this->get_logger(), "Received empty image from cv_bridge");
                return;
            }

            // Run TensorRT inference
            run_inference(color_image);

            // Display detection results
            cv::Mat detection;
            {
                std::lock_guard<std::mutex> lock(display_mutex_);
                detection = latest_detection_.clone();
            }

            if (!detection.empty()) {
                cv::imshow("Detections (TensorRT)", detection);
            }

            int key = cv::waitKey(1);
            if (key == 27) {  // ESC key to exit
                RCLCPP_INFO(this->get_logger(), "ESC key pressed, exiting");
                rclcpp::shutdown();
            }
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        } catch (const std::exception& e) {
            RCLCPP_ERROR(this->get_logger(), "Inference exception: %s", e.what());
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

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr colorCameraFeed_;
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

    // Display state
    cv::Mat latest_detection_;
    std::mutex display_mutex_;
};


int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CudaNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
