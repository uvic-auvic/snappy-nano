// Shared multi-camera TensorRT inference node.
//
// Replaces the previous per-camera executables (front_cam / bottom_cam), which
// each ran a full, duplicated TensorRT pipeline in its own process. Running two
// processes means two CUDA contexts, two deserialized engines and two copies of
// the model weights resident in the Orin Nano's shared DRAM at all times.
//
// This node merges everything into a single process:
//   * one CUDA context, one engine, one execution context, one CUDA stream
//   * one set of GPU I/O buffers (zero-copy mapped on integrated Jetson memory)
//   * N independent camera subscriptions (kept separate, per requirement)
//   * a single dedicated inference worker thread fed by a bounded job queue
//   * dynamic-batch inference: frames waiting in the queue (typically one per
//     camera) are coalesced into a single batched enqueueV3 call
//   * GPU preprocessing: a fused CUDA kernel does letterbox + BGR->RGB +
//     normalize + HWC->CHW, so the CPU only uploads the raw frame
//
// The GPU is a single serial resource, so preprocessing + inference are funneled
// through one worker thread / one stream. The wins versus the old design: the
// 6-core CPU is freed from all per-frame image math, multiple cameras share one
// inference launch via batching, and there is exactly one of every expensive
// GPU allocation regardless of how many cameras are added.
//
// BATCHING REQUIRES a dynamic batch axis in the model. If the .engine / .onnx
// exposes input shape [-1, 3, H, W], this node builds an optimization profile
// (batch 1..max_batch), sizes its buffers for max_batch, and batches frames.
// If the model is static batch=1, the node detects that and transparently falls
// back to single-frame inference -- no crash, just no batching. To actually get
// batching, export the ONNX with a dynamic batch dimension, e.g. (ultralytics):
//   model.export(format="onnx", dynamic=True, batch=<max_batch>)
//
// Scaling to more cameras is a launch-file change (add a namespace to the
// `camera_namespaces` list) -- no new binary, context or engine.
//
// NOTE on TensorRT teardown: this targets the TensorRT shipped with JetPack 6.2,
// where the correct way to release nvinfer objects is `delete` (the destructor),
// NOT the older `->destroy()` API. Do not reintroduce `->destroy()` here.

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
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cmath>
#include <sstream>

#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_cpp/msg/object_detection.hpp"
#include "snappy_cpp/msg/bounding_box2_d.hpp"

#include "preprocess_cuda.h"

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

class CameraInferenceNode : public rclcpp::Node
{
public:
    CameraInferenceNode() : Node("camera_inference")
    {
        RCLCPP_INFO(get_logger(), "=== Shared CUDA + TensorRT YOLO Node ===");

        // Camera namespaces drive the whole node. Adding a camera = add a name.
        this->declare_parameter<std::vector<std::string>>(
            "camera_namespaces", std::vector<std::string>{"d455", "d405"});
        this->declare_parameter<double>("inference_hz", 10.0);
        this->declare_parameter<bool>("display", false);
        this->declare_parameter<int>("distance_samples", 100);
        this->declare_parameter<int>("queue_depth", 0);
        // 0 => default to one slot per camera (set below once we know the count).
        this->declare_parameter<int>("max_batch", 0);
        // 0 => default to number of cameras (batch all cameras together).
        this->declare_parameter<double>("batch_collect_ms", 0.0);
        // >0 => after the first job, wait up to this long for more frames to
        // fill the batch. 0 => opportunistic (batch only what is already queued,
        // adding zero latency).

        this->declare_parameter<bool>("save_images", true);
        this->declare_parameter<std::string>("save_dir", std::string(getenv("HOME")) + "/Desktop/snappy_inference");


        camera_namespaces_ = this->get_parameter("camera_namespaces").as_string_array();
        inference_hz_ = this->get_parameter("inference_hz").as_double();
        display_ = this->get_parameter("display").as_bool();
        save_images_ = this->get_parameter("save_images").as_bool();
        save_dir_ = this->get_parameter("save_dir").as_string();
        distance_samples_ = std::max(1, static_cast<int>(this->get_parameter("distance_samples").as_int()));
        distance_grid_ = std::max(1, static_cast<int>(std::ceil(std::sqrt(static_cast<double>(distance_samples_)))));

        if (camera_namespaces_.empty()) {
            throw std::runtime_error("camera_namespaces parameter is empty");
        }

        const int n_cam = static_cast<int>(camera_namespaces_.size());
        const int max_batch_param = this->get_parameter("max_batch").as_int();
        requested_max_batch_ = max_batch_param > 0 ? max_batch_param : n_cam;
        const int queue_depth_param = this->get_parameter("queue_depth").as_int();
        max_queue_depth_ = std::max(requested_max_batch_,
                                    queue_depth_param > 0 ? queue_depth_param : n_cam);
        batch_collect_ = std::chrono::duration<double, std::milli>(
            std::max(0.0, this->get_parameter("batch_collect_ms").as_double()));

        RCLCPP_INFO(get_logger(),
            "Settings: inference_hz=%.1f, display=%s, distance_samples=%d, queue_depth=%d, "
            "max_batch(requested)=%d, batch_collect_ms=%.1f",
            inference_hz_, display_ ? "true" : "false", distance_samples_, max_queue_depth_,
            requested_max_batch_, batch_collect_.count());

        if (!save_images_) {
            RCLCPP_WARN(get_logger(), "Image saving disabled (set save_images=true to enable)");
        } else {
            const std::string base = save_dir_.empty()
                ? (std::filesystem::current_path().string() + "/../<camera>")
                : (save_dir_ + "/<camera>");
            RCLCPP_INFO(get_logger(), "Saving images under: %s", base.c_str());
        }

        checkCUDA();
        build_engine_from_onnx();
        allocate_buffers();
        setup_cameras();

        // Start the single inference worker before spinning so it is ready for
        // the first synchronized frame.
        worker_thread_ = std::thread(&CameraInferenceNode::inference_worker, this);

        RCLCPP_INFO(get_logger(), "Shared inference node ready for %zu camera(s)", cameras_.size());
    }

    ~CameraInferenceNode() override
    {
        // 1. Stop the worker thread and let it drain.
        stop_ = true;
        queue_cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }

        if (display_) {
            for (auto& cam : cameras_) {
                cv::destroyWindow(cam->display_window_name);
            }
        }

        // 2. Release TensorRT objects via delete (JetPack 6.2 convention --
        //    NOT ->destroy()). Order: context, then engine, then runtime.
        if (context_ != nullptr) {
            delete context_;
            context_ = nullptr;
        }
        if (engine_ != nullptr) {
            delete engine_;
            engine_ = nullptr;
        }
        if (runtime_ != nullptr) {
            delete runtime_;
            runtime_ = nullptr;
        }

        // 3. Release CUDA buffers. For zero-copy, the device I/O pointers are
        //    aliases of the mapped host buffers and must NOT be cudaFree'd
        //    separately. For the copy path, input/output live on the device and
        //    only the output has a pinned host staging buffer.
        if (use_zero_copy_) {
            if (host_input_ != nullptr) cudaFreeHost(host_input_);
            if (host_output_ != nullptr) cudaFreeHost(host_output_);
        } else {
            if (device_input_ != nullptr) cudaFree(device_input_);
            if (device_output_ != nullptr) cudaFree(device_output_);
            if (host_output_ != nullptr) cudaFreeHost(host_output_);
        }
        host_input_ = host_output_ = nullptr;
        device_input_ = device_output_ = nullptr;

        if (d_src_ != nullptr) {
            cudaFree(d_src_);
            d_src_ = nullptr;
        }

        if (stream_ != nullptr) {
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
    }

private:
    struct Detection {
        cv::Rect2f box;   // x, y, width, height in original image pixels
        float conf;
        int   class_id;
        float distance_m = -1.0f;
    };

    struct DetectionResult {
        std::vector<Detection> detections;
        float inference_time_ms;
        rclcpp::Time timestamp;
    };

    static constexpr float CONF_THRESH = 0.25f;  // NMS-free head
    static constexpr int NUM_CLASSES = 80;

    typedef message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image, sensor_msgs::msg::Image> SyncPolicy;

    // Per-camera state. Each camera keeps its own subscriptions, synchronizer,
    // publisher, callback group and rate-limit clock -- the streams stay fully
    // separate; only the GPU pipeline is shared.
    struct CameraContext {
        size_t index = 0;
        std::string ns;
        std::string display_window_name;
        rclcpp::CallbackGroup::SharedPtr cb_group;
        message_filters::Subscriber<sensor_msgs::msg::Image> color_sub;
        message_filters::Subscriber<sensor_msgs::msg::Image> depth_sub;
        std::shared_ptr<message_filters::Synchronizer<SyncPolicy>> sync;
        rclcpp::Publisher<snappy_cpp::msg::DetectionArray>::SharedPtr pub;
        std::optional<rclcpp::Time> last_inference_time;  // touched only by this camera's (mutex-excl) callback
    };

    // A unit of work handed to the inference worker. The cv_bridge const-shared
    // pointers keep the underlying ROS message buffers alive, so the cv::Mats
    // reference image data without copying it.
    struct InferenceJob {
        size_t cam_index = 0;
        cv_bridge::CvImageConstPtr color_ptr;
        cv_bridge::CvImageConstPtr depth_ptr;
        rclcpp::Time timestamp;
    };

    class TRTLogger : public nvinfer1::ILogger {
    public:
        void log(Severity severity, const char* msg) noexcept override {
            if (severity <= Severity::kWARNING) {
                std::cerr << "[TensorRT] " << msg << std::endl;
            }
        }
    };

    struct TRTDestroyer {
        template <typename T>
        void operator()(T* object) const
        {
            if (object != nullptr) {
                delete object;  // JetPack 6.2 TensorRT: delete, not ->destroy()
            }
        }
    };

    void checkCUDA() {
        int deviceCount = 0;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess) {
            RCLCPP_ERROR(get_logger(), "Failed to get device count: %s", cudaGetErrorString(err));
            throw std::runtime_error("CUDA unavailable");
        }
        RCLCPP_INFO(get_logger(), "Found %d CUDA devices", deviceCount);
        if (deviceCount == 0) {
            throw std::runtime_error("No CUDA devices found");
        }

        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        RCLCPP_INFO(get_logger(), "GPU: %s", prop.name);
        RCLCPP_INFO(get_logger(), "Compute capability: %d.%d", prop.major, prop.minor);
        RCLCPP_INFO(get_logger(), "GPU Memory: %zu MB", prop.totalGlobalMem / (1024 * 1024));
        RCLCPP_INFO(get_logger(), "Integrated: %s, canMapHostMemory: %s",
                    prop.integrated ? "yes" : "no", prop.canMapHostMemory ? "yes" : "no");

        // On integrated Jetson memory, CPU and GPU share the same physical DRAM,
        // so mapped pinned memory lets the engine read/write host buffers
        // directly -- no H2D/D2H copies. Enable it before any context exists.
        if (prop.integrated && prop.canMapHostMemory) {
            cudaError_t flag_err = cudaSetDeviceFlags(cudaDeviceMapHost);
            if (flag_err == cudaSuccess || flag_err == cudaErrorSetOnActiveProcess) {
                use_zero_copy_ = true;
                RCLCPP_INFO(get_logger(), "Zero-copy mapped I/O enabled (integrated memory)");
            } else {
                RCLCPP_WARN(get_logger(), "cudaSetDeviceFlags(MapHost) failed: %s -- using explicit copies",
                            cudaGetErrorString(flag_err));
            }
        } else {
            RCLCPP_INFO(get_logger(), "Discrete/unmapped GPU -- using pinned host + explicit async copies");
        }
        RCLCPP_INFO(get_logger(), "CUDA: OK");
    }

    // Product of every dimension except the batch axis (index 0). Used to size
    // per-sample slices independently of the (possibly dynamic) batch dim.
    static size_t volume_excluding_batch(const nvinfer1::Dims& dims)
    {
        size_t result = 1;
        for (int i = 1; i < dims.nbDims; ++i) {
            if (dims.d[i] <= 0) {
                return 0;  // unexpected dynamic non-batch dim
            }
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
        std::string engine_path;
        std::string cache_dir = std::string(getenv("HOME")) + "/.cache/snappy_cpp";
        // Cache key includes the requested max batch: a batch-2 build and a
        // batch-1 build are different engines and must not alias on disk.
        std::string cached_engine_path =
            cache_dir + "/yolo26s_b" + std::to_string(requested_max_batch_) + ".engine";
        bool has_engine = false;

        try {
            engine_path = resolve_model_path("engine");
            has_engine = true;
            RCLCPP_INFO(get_logger(), "Found pre-built TensorRT engine: %s", engine_path.c_str());
        } catch (...) {
            if (std::filesystem::exists(cached_engine_path)) {
                engine_path = cached_engine_path;
                has_engine = true;
                RCLCPP_INFO(get_logger(), "Found cached TensorRT engine: %s", engine_path.c_str());
            } else {
                RCLCPP_WARN(get_logger(), "No pre-built .engine file found, will build from ONNX");
            }
        }

        // The runtime must outlive every engine it deserializes, so it is a
        // member (the old per-camera code let it die at function scope, which
        // is fragile across TensorRT versions).
        runtime_ = nvinfer1::createInferRuntime(trt_logger_);
        if (!runtime_) {
            throw std::runtime_error("Failed to create TensorRT runtime");
        }

        if (has_engine) {
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

            engine_ = runtime_->deserializeCudaEngine(engine_data.data(), engine_size);
            if (!engine_) {
                throw std::runtime_error("Failed to deserialize pre-built TensorRT engine");
            }
            RCLCPP_INFO(get_logger(), "Loaded pre-built TensorRT engine");
        } else {
            const std::string model_path = resolve_model_path("onnx");
            RCLCPP_INFO(get_logger(), "Building TensorRT engine from ONNX: %s", model_path.c_str());
            RCLCPP_INFO(get_logger(), "This may take 2-5 minutes on first run...");

            auto builder = std::unique_ptr<nvinfer1::IBuilder, TRTDestroyer>(
                nvinfer1::createInferBuilder(trt_logger_));
            if (!builder) {
                throw std::runtime_error("Failed to create TensorRT builder");
            }

            // TensorRT 10 networks are always explicit-batch; the
            // kEXPLICIT_BATCH flag is deprecated, so create with no flags.
            auto network = std::unique_ptr<nvinfer1::INetworkDefinition, TRTDestroyer>(
                builder->createNetworkV2(0));
            if (!network) {
                throw std::runtime_error("Failed to create TensorRT network");
            }

            auto parser = std::unique_ptr<nvonnxparser::IParser, TRTDestroyer>(
                nvonnxparser::createParser(*network, trt_logger_));
            if (!parser) {
                throw std::runtime_error("Failed to create TensorRT ONNX parser");
            }
            if (!parser->parseFromFile(model_path.c_str(),
                                       static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
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

            // If the ONNX input has a dynamic batch axis (d[0] == -1), add an
            // optimization profile spanning batch 1..requested_max_batch so the
            // built engine can run any batch in that range. If the ONNX is
            // static (fixed batch), we leave it untouched -- batching simply
            // won't be available and the node falls back to single-frame.
            nvinfer1::ITensor* net_input = network->getInput(0);
            nvinfer1::Dims net_dims = net_input->getDimensions();
            if (net_dims.nbDims == 4 && net_dims.d[0] == -1 &&
                net_dims.d[2] > 0 && net_dims.d[3] > 0) {
                const int h = net_dims.d[2];
                const int w = net_dims.d[3];
                auto* profile = builder->createOptimizationProfile();
                profile->setDimensions(net_input->getName(), nvinfer1::OptProfileSelector::kMIN,
                                       nvinfer1::Dims4(1, 3, h, w));
                profile->setDimensions(net_input->getName(), nvinfer1::OptProfileSelector::kOPT,
                                       nvinfer1::Dims4(requested_max_batch_, 3, h, w));
                profile->setDimensions(net_input->getName(), nvinfer1::OptProfileSelector::kMAX,
                                       nvinfer1::Dims4(requested_max_batch_, 3, h, w));
                config->addOptimizationProfile(profile);
                RCLCPP_INFO(get_logger(),
                    "Dynamic ONNX detected: building batch profile 1..%d", requested_max_batch_);
            } else {
                RCLCPP_WARN(get_logger(),
                    "ONNX input batch is static -- engine will be batch=1 (no batching). "
                    "Re-export with a dynamic batch axis to enable batching.");
            }

            auto serialized_network = std::unique_ptr<nvinfer1::IHostMemory, TRTDestroyer>(
                builder->buildSerializedNetwork(*network, *config));
            if (!serialized_network) {
                throw std::runtime_error("Failed to serialize TensorRT network");
            }

            std::filesystem::create_directories(cache_dir);
            std::ofstream cache_file(cached_engine_path, std::ios::binary);
            if (cache_file) {
                cache_file.write(static_cast<const char*>(serialized_network->data()),
                                 serialized_network->size());
                cache_file.close();
                RCLCPP_INFO(get_logger(), "Cached TensorRT engine to: %s", cached_engine_path.c_str());
            } else {
                RCLCPP_WARN(get_logger(), "Failed to cache engine (non-fatal)");
            }

            engine_ = runtime_->deserializeCudaEngine(serialized_network->data(),
                                                      serialized_network->size());
            if (!engine_) {
                throw std::runtime_error("Failed to deserialize TensorRT engine");
            }
            RCLCPP_INFO(get_logger(), "TensorRT engine built successfully");
        }

        context_ = engine_->createExecutionContext();
        if (!context_) {
            throw std::runtime_error("Failed to create TensorRT execution context");
        }

        int32_t nb_tensors = engine_->getNbIOTensors();
        for (int32_t i = 0; i < nb_tensors; ++i) {
            const char* tensor_name = engine_->getIOTensorName(i);
            nvinfer1::TensorIOMode io_mode = engine_->getTensorIOMode(tensor_name);
            if (io_mode == nvinfer1::TensorIOMode::kINPUT) {
                input_name_ = tensor_name;
            } else if (io_mode == nvinfer1::TensorIOMode::kOUTPUT) {
                output_name_ = tensor_name;
            }
        }
        if (input_name_.empty() || output_name_.empty()) {
            throw std::runtime_error("Could not resolve TensorRT input/output tensors");
        }

        nvinfer1::Dims in_dims = engine_->getTensorShape(input_name_.c_str());
        if (in_dims.nbDims != 4) {
            throw std::runtime_error("Expected 4-D NCHW input tensor");
        }
        if (in_dims.d[2] > 0 && in_dims.d[3] > 0) {
            input_h_ = in_dims.d[2];
            input_w_ = in_dims.d[3];
        }

        // Discover the engine's real batch capability (source of truth), whether
        // it came from the ONNX build above or a pre-built/cached .engine.
        if (in_dims.d[0] == -1) {
            dynamic_engine_ = true;
            nvinfer1::Dims max_dims = engine_->getProfileShape(
                input_name_.c_str(), 0, nvinfer1::OptProfileSelector::kMAX);
            engine_max_batch_ = max_dims.d[0] > 0 ? static_cast<int>(max_dims.d[0]) : 1;
        } else {
            dynamic_engine_ = false;
            engine_max_batch_ = in_dims.d[0] > 0 ? static_cast<int>(in_dims.d[0]) : 1;
        }
        effective_max_batch_ = std::max(1, std::min(requested_max_batch_, engine_max_batch_));

        // Per-sample element counts (product of all dims except batch). These are
        // batch-independent, so they hold for any batch size at run time.
        single_input_count_ = volume_excluding_batch(in_dims);
        nvinfer1::Dims out_dims = engine_->getTensorShape(output_name_.c_str());
        single_output_count_ = volume_excluding_batch(out_dims);
        if (single_input_count_ == 0 || single_output_count_ == 0) {
            throw std::runtime_error("Failed to resolve per-sample tensor sizes");
        }

        // Static engines run a single immutable batch size: we cannot choose it,
        // so pin to whatever the engine was built with and set that shape once.
        if (!dynamic_engine_) {
            effective_max_batch_ = engine_max_batch_;  // typically 1
            if (!context_->setInputShape(input_name_.c_str(), in_dims)) {
                throw std::runtime_error("Failed to set static TensorRT input shape");
            }
        }

        RCLCPP_INFO(get_logger(),
            "TensorRT ready. Input %dx%d | dynamic=%s | engine_max_batch=%d | "
            "effective_max_batch=%d | per-sample out=%zu",
            input_w_, input_h_, dynamic_engine_ ? "yes" : "no",
            engine_max_batch_, effective_max_batch_, single_output_count_);
        if (effective_max_batch_ == 1) {
            RCLCPP_WARN(get_logger(),
                "Running batch=1 (engine does not support batching) -- cameras are "
                "processed one frame at a time.");
        }
    }

    // Allocate the single shared I/O buffer set. With zero-copy the device
    // pointers are aliases into mapped host memory; otherwise we keep pinned
    // host staging buffers (for true async copies) plus device buffers.
    void allocate_buffers()
    {
        if (cudaStreamCreate(&stream_) != cudaSuccess) {
            throw std::runtime_error("Failed to create CUDA stream");
        }

        // Size buffers for the largest batch the engine can run, so a single
        // contiguous allocation covers every batch size at run time.
        const size_t max_in_count = single_input_count_ * static_cast<size_t>(effective_max_batch_);
        const size_t max_out_count = single_output_count_ * static_cast<size_t>(effective_max_batch_);
        const size_t in_bytes = max_in_count * sizeof(float);
        const size_t out_bytes = max_out_count * sizeof(float);

        // The preprocessing kernel writes the engine input on the device, so we
        // never stage the (float, CHW) input on the host -- host_input_ only
        // exists in zero-copy mode as the mapped backing of device_input_.
        if (use_zero_copy_) {
            if (cudaHostAlloc(reinterpret_cast<void**>(&host_input_), in_bytes,
                              cudaHostAllocMapped) != cudaSuccess ||
                cudaHostAlloc(reinterpret_cast<void**>(&host_output_), out_bytes,
                              cudaHostAllocMapped) != cudaSuccess) {
                throw std::runtime_error("Failed to allocate mapped host buffers");
            }
            if (cudaHostGetDevicePointer(reinterpret_cast<void**>(&device_input_), host_input_, 0) != cudaSuccess ||
                cudaHostGetDevicePointer(reinterpret_cast<void**>(&device_output_), host_output_, 0) != cudaSuccess) {
                throw std::runtime_error("Failed to map host buffers to device pointers");
            }
        } else {
            // Input: device-only (kernel target). Output: device + pinned host
            // staging for a true async D2H copy.
            if (cudaMalloc(reinterpret_cast<void**>(&device_input_), in_bytes) != cudaSuccess ||
                cudaMalloc(reinterpret_cast<void**>(&device_output_), out_bytes) != cudaSuccess) {
                throw std::runtime_error("Failed to allocate device buffers");
            }
            if (cudaHostAlloc(reinterpret_cast<void**>(&host_output_), out_bytes,
                              cudaHostAllocDefault) != cudaSuccess) {
                throw std::runtime_error("Failed to allocate pinned host output buffer");
            }
        }

        // Tensor addresses are fixed for the lifetime of the node (single shared
        // buffer set), so bind them once instead of per inference.
        context_->setTensorAddress(input_name_.c_str(), device_input_);
        context_->setTensorAddress(output_name_.c_str(), device_output_);
    }

    void setup_cameras()
    {
        cameras_.reserve(camera_namespaces_.size());
        for (size_t i = 0; i < camera_namespaces_.size(); ++i) {
            auto cam = std::make_unique<CameraContext>();
            cam->index = i;
            cam->ns = camera_namespaces_[i];
            cam->display_window_name = "Detections (TensorRT) [" + cam->ns + "]";

            // Each camera gets a mutually-exclusive callback group so its own
            // sync callback never runs concurrently with itself, while different
            // cameras can enqueue in parallel under the MultiThreadedExecutor.
            cam->cb_group = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
            rclcpp::SubscriptionOptions sub_opts;
            sub_opts.callback_group = cam->cb_group;

            std::string color_topic = "/" + cam->ns + "/color/image_raw";
            if (cam->ns == "d405") {
                color_topic = "/" + cam->ns + "/color/image_rect_raw";
            }
            const std::string depth_topic = "/" + cam->ns + "/aligned_depth_to_color/image_raw";
            const std::string detection_topic = "/" + cam->ns + "/detections";

            cam->pub = this->create_publisher<snappy_cpp::msg::DetectionArray>(
                detection_topic, rclcpp::SensorDataQoS());

            cam->color_sub.subscribe(this, color_topic, rmw_qos_profile_default, sub_opts);
            cam->depth_sub.subscribe(this, depth_topic, rmw_qos_profile_default, sub_opts);
            cam->sync = std::make_shared<message_filters::Synchronizer<SyncPolicy>>(
                SyncPolicy(10), cam->color_sub, cam->depth_sub);

            // message_filters' Synchronizer doesn't reliably bind capturing
            // lambdas, so use std::bind to a member function (the cam index is
            // pre-bound; the two messages come from the placeholders).
            const size_t cam_index = i;
            cam->sync->registerCallback(
                std::bind(&CameraInferenceNode::sync_callback, this, cam_index,
                          std::placeholders::_1, std::placeholders::_2));

            if (display_) {
                cv::namedWindow(cam->display_window_name, cv::WINDOW_AUTOSIZE);
            }

            RCLCPP_INFO(get_logger(), "Camera '%s': color=%s depth=%s -> %s",
                        cam->ns.c_str(), color_topic.c_str(), depth_topic.c_str(), detection_topic.c_str());
            cameras_.push_back(std::move(cam));
        }
    }
    // Generate a unique image filename based on the current time.
    static std::string name_image(const std::string& base_dir,
                                  const std::string& type,
                                  const std::string& ext = "jpg")
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        time_t tt = system_clock::to_time_t(now);

        // Milliseconds for extra uniqueness
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        // Thread-safe localtime
        tm tm{};
        #ifdef _WIN32
            localtime_s(&tm, &tt);
        #else
            localtime_r(&tt, &tm);
        #endif

        const std::string dir = base_dir.empty() ? (std::string(getenv("HOME")) + "/Desktop/snappy_inference/" + type) : (base_dir + "/" + type);
        std::filesystem::create_directories(dir);

        std::ostringstream oss;
        oss << dir << "/"
            << std::put_time(&tm, "%Y%m%d_%H%M%S")
            << '_' << std::setw(3) << std::setfill('0') << ms.count()
            << '.' << ext;
        return oss.str();
    }

    // --- Subscription side (runs on executor threads) ---------------------
    // Kept deliberately light: rate-gate, wrap the frames, enqueue, return.
    void save_training_image(const std::string& cam_name, const cv::Mat& image, const rclcpp::Time& timestamp)
    {
        if (!save_images_) return;
        (void)timestamp;
        if (image.empty()) {
            RCLCPP_WARN(get_logger(), "Skipping save for %s (empty image)", cam_name.c_str());
            return;
        }
        const std::string filename = name_image(save_dir_, cam_name, "jpg");
        if (!cv::imwrite(filename, image)) {
            RCLCPP_ERROR(get_logger(), "Failed to save image to %s", filename.c_str());
            return;
        }
        const uint64_t saved = saved_images_.fetch_add(1) + 1;
        if (saved == 1) {
            RCLCPP_INFO(get_logger(), "First image saved: %s", filename.c_str());
        }
    }

    void sync_callback(size_t cam_index,
                       const sensor_msgs::msg::Image::ConstSharedPtr& color_msg,
                       const sensor_msgs::msg::Image::ConstSharedPtr& depth_msg)
    {
        if (!color_msg || !depth_msg) {
            RCLCPP_ERROR(get_logger(), "Received null image message");
            return;
        }

        CameraContext& cam = *cameras_[cam_index];

        // Per-camera rate limit, checked before any conversion so dropped frames
        // cost almost nothing.
        const rclcpp::Time now = this->get_clock()->now();
        const double min_period = inference_hz_ > 0.0 ? (1.0 / inference_hz_) : 0.0;
        if (min_period > 0.0 && cam.last_inference_time.has_value() &&
            (now - *cam.last_inference_time).seconds() < min_period) {
            return;
        }
        cam.last_inference_time = now;

        try {
            InferenceJob job;
            job.cam_index = cam_index;
            // toCvShare avoids copying the image data; the returned pointers keep
            // the ROS messages alive for as long as the job exists.
            job.color_ptr = cv_bridge::toCvShare(color_msg, "bgr8");
            job.depth_ptr = cv_bridge::toCvShare(depth_msg, "16UC1");
            job.timestamp = rclcpp::Time(color_msg->header.stamp.sec, color_msg->header.stamp.nanosec);

            save_training_image(cam.ns, job.color_ptr->image, job.timestamp);

            enqueue_job(std::move(job));
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void enqueue_job(InferenceJob&& job)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            // Bounded, drop-oldest: never accumulate latency if the GPU can't
            // keep up with the camera frame rate.
            while (job_queue_.size() >= static_cast<size_t>(max_queue_depth_)) {
                job_queue_.pop_front();
                dropped_jobs_++;
            }
            job_queue_.push_back(std::move(job));
        }
        queue_cv_.notify_one();
    }

    // --- Inference side (single dedicated worker thread) ------------------
    // Drains up to effective_max_batch_ jobs and runs them as one batched
    // inference. Frames queued by different cameras are coalesced into a single
    // enqueueV3 launch.
    void inference_worker()
    {
        std::vector<InferenceJob> batch;
        batch.reserve(static_cast<size_t>(effective_max_batch_));

        while (true) {
            batch.clear();
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this] { return stop_ || !job_queue_.empty(); });
                if (stop_ && job_queue_.empty()) {
                    return;
                }
                drain_locked(batch);
            }

            // Optional coalescing window: briefly wait for the rest of the batch
            // to arrive (e.g. the other camera's frame) before committing to the
            // GPU. Disabled (0 ms) by default so we never add latency.
            if (batch_collect_.count() > 0.0 &&
                batch.size() < static_cast<size_t>(effective_max_batch_) && !stop_) {
                const auto deadline = std::chrono::steady_clock::now() + batch_collect_;
                std::unique_lock<std::mutex> lock(queue_mutex_);
                while (batch.size() < static_cast<size_t>(effective_max_batch_) && !stop_) {
                    if (!queue_cv_.wait_until(lock, deadline,
                            [this] { return stop_ || !job_queue_.empty(); })) {
                        break;  // timed out
                    }
                    drain_locked(batch);
                }
            }

            try {
                process_batch(batch);
            } catch (const std::exception& e) {
                RCLCPP_ERROR(get_logger(), "Inference worker exception: %s", e.what());
            }
        }
    }

    // Move up to effective_max_batch_ jobs from the queue into `batch`.
    // Caller must hold queue_mutex_.
    void drain_locked(std::vector<InferenceJob>& batch)
    {
        while (!job_queue_.empty() &&
               batch.size() < static_cast<size_t>(effective_max_batch_)) {
            batch.push_back(std::move(job_queue_.front()));
            job_queue_.pop_front();
        }
    }

    void process_batch(std::vector<InferenceJob>& jobs)
    {
        // Drop any jobs with frames the kernel can't consume (empty or not the
        // expected 8-bit BGR layout) up front so indices stay aligned with the
        // batch slices we feed the engine.
        jobs.erase(std::remove_if(jobs.begin(), jobs.end(),
            [this](const InferenceJob& j) {
                const cv::Mat& img = j.color_ptr->image;
                if (img.empty() || img.type() != CV_8UC3) {
                    RCLCPP_ERROR(get_logger(), "Dropping frame (empty or not 8-bit BGR)");
                    return true;
                }
                return false;
            }), jobs.end());
        if (jobs.empty()) {
            return;
        }

        const size_t B = jobs.size();
        std::vector<float> scales(B);
        std::vector<int> pad_x(B), pad_y(B);

        auto t0 = std::chrono::steady_clock::now();

        // Ensure the reusable device source buffer fits the largest frame in the
        // batch before any upload, so we never realloc while a kernel that reads
        // it is still in flight on the stream.
        size_t max_src_bytes = 0;
        for (const auto& j : jobs) {
            const cv::Mat& img = j.color_ptr->image;
            max_src_bytes = std::max(max_src_bytes, img.step[0] * static_cast<size_t>(img.rows));
        }
        ensure_src_capacity(max_src_bytes);

        // GPU preprocessing: upload each raw BGR frame to the device, then a
        // fused kernel does letterbox + BGR->RGB + normalize + HWC->CHW straight
        // into the engine input slice. All ops are enqueued on stream_ in order,
        // so the single reusable source buffer is safe across samples.
        for (size_t b = 0; b < B; ++b) {
            const cv::Mat& img = jobs[b].color_ptr->image;
            compute_letterbox_params(img.cols, img.rows, scales[b], pad_x[b], pad_y[b]);

            const size_t src_bytes = img.step[0] * static_cast<size_t>(img.rows);
            if (cudaMemcpyAsync(d_src_, img.data, src_bytes,
                                cudaMemcpyHostToDevice, stream_) != cudaSuccess) {
                RCLCPP_ERROR(get_logger(), "Source image H2D upload failed");
                return;
            }

            cudaError_t launch_err = launchLetterboxPreprocess(
                d_src_, img.cols, img.rows, img.step[0],
                device_input_ + b * single_input_count_, input_w_, input_h_,
                scales[b], pad_x[b], pad_y[b], stream_);
            if (launch_err != cudaSuccess) {
                RCLCPP_ERROR(get_logger(), "Preprocess kernel launch failed: %s",
                             cudaGetErrorString(launch_err));
                return;
            }
        }

        if (!run_batched_inference(B)) {
            return;
        }

        auto t1 = std::chrono::steady_clock::now();
        const float batch_ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

        // Demux: parse each sample's output slice and route to its camera.
        for (size_t b = 0; b < B; ++b) {
            const cv::Mat& color_image = jobs[b].color_ptr->image;
            const cv::Mat& depth_image = jobs[b].depth_ptr->image;

            DetectionResult det_result;
            det_result.timestamp = jobs[b].timestamp;
            det_result.inference_time_ms = batch_ms;
            det_result.detections = parse_detections(
                host_output_ + b * single_output_count_,
                color_image.cols, color_image.rows, scales[b], pad_x[b], pad_y[b]);

            for (auto& detection : det_result.detections) {
                detection.distance_m = distance_away(detection, depth_image);
            }

            CameraContext& cam = *cameras_[jobs[b].cam_index];
            publish_detections(cam, det_result);
            if (display_) {
                render_display(cam, color_image, det_result);
            }
        }
    }

    // Letterbox mapping (aspect-preserving fit into input_w_ x input_h_ with
    // centered padding). The actual resampling happens in the CUDA kernel; this
    // just produces the scale/pad the kernel and the box de-projection share.
    void compute_letterbox_params(int src_w, int src_h, float& scale, int& pad_x, int& pad_y)
    {
        const float ratio = std::min(float(input_w_) / src_w, float(input_h_) / src_h);
        const int new_w = int(src_w * ratio);
        const int new_h = int(src_h * ratio);
        pad_x = (input_w_ - new_w) / 2;
        pad_y = (input_h_ - new_h) / 2;
        scale = ratio;
    }

    // Grow the reusable device buffer that holds the raw source image for the
    // preprocessing kernel. Only ever called when no kernel is in flight.
    void ensure_src_capacity(size_t bytes)
    {
        if (bytes <= src_capacity_) {
            return;
        }
        if (d_src_ != nullptr) {
            cudaFree(d_src_);
            d_src_ = nullptr;
        }
        if (cudaMalloc(reinterpret_cast<void**>(&d_src_), bytes) != cudaSuccess) {
            src_capacity_ = 0;
            throw std::runtime_error("Failed to allocate device source buffer");
        }
        src_capacity_ = bytes;
    }

    std::vector<Detection> parse_detections(const float* output_data,
        int originalWidth, int originalHeight, float scale, int paddingX, int paddingY)
    {
        std::vector<Detection> result;
        result.reserve(300);

        // YOLO26 output: (1, 300, 6) -> [x1, y1, x2, y2, confidence, class_id]
        const int num_detections = 300;
        const int values_per_detection = 6;

        for (int i = 0; i < num_detections; ++i) {
            const int offset = i * values_per_detection;

            float conf = output_data[offset + 4];
            if (conf < CONF_THRESH) continue;

            int class_id = static_cast<int>(output_data[offset + 5]);
            if (class_id < 0 || class_id >= NUM_CLASSES) continue;

            float x1 = output_data[offset + 0];
            float y1 = output_data[offset + 1];
            float x2 = output_data[offset + 2];
            float y2 = output_data[offset + 3];

            // Undo letterbox padding, scale back to original image pixels.
            x1 = (x1 - paddingX) / scale;
            y1 = (y1 - paddingY) / scale;
            x2 = (x2 - paddingX) / scale;
            y2 = (y2 - paddingY) / scale;

            x1 = std::clamp(x1, 0.f, (float)originalWidth);
            y1 = std::clamp(y1, 0.f, (float)originalHeight);
            x2 = std::clamp(x2, 0.f, (float)originalWidth);
            y2 = std::clamp(y2, 0.f, (float)originalHeight);

            if (x2 <= x1 || y2 <= y1) continue;

            result.push_back({{x1, y1, x2 - x1, y2 - y1}, conf, class_id, -1.0f});
        }
        return result;
    }

    float distance_away(const Detection& d, const cv::Mat& depth_image)
    {
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
        const int grid = distance_grid_;
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

        const size_t mid_idx = depth_values.size() / 2;
        auto mid = depth_values.begin() + static_cast<std::ptrdiff_t>(mid_idx);
        std::nth_element(depth_values.begin(), mid, depth_values.end());

        if ((depth_values.size() % 2U) != 0U) {
            return static_cast<float>(*mid) / 1000.0f;
        }
        auto lower_max = std::max_element(depth_values.begin(), mid);
        const float even_median_mm = (static_cast<float>(*lower_max) + static_cast<float>(*mid)) * 0.5f;
        return even_median_mm / 1000.0f;
    }

    // Run inference over `batch_size` frames. The preprocessing kernels have
    // already written device_input_ (slice b at b * single_input_count_) earlier
    // on stream_, so there is no input copy here -- just the (optional) dynamic
    // shape, the launch, and the output read-back. Tensor addresses are bound
    // once at allocation.
    bool run_batched_inference(size_t batch_size)
    {
        const size_t out_count = batch_size * single_output_count_;

        // Dynamic engines need the batch dimension set before each launch.
        if (dynamic_engine_) {
            nvinfer1::Dims shape;
            shape.nbDims = 4;
            shape.d[0] = static_cast<int32_t>(batch_size);
            shape.d[1] = 3;
            shape.d[2] = input_h_;
            shape.d[3] = input_w_;
            if (!context_->setInputShape(input_name_.c_str(), shape)) {
                RCLCPP_ERROR(get_logger(), "Failed to set input shape for batch=%zu", batch_size);
                return false;
            }
        }

        if (!context_->enqueueV3(stream_)) {
            RCLCPP_ERROR(get_logger(), "TensorRT enqueue failed");
            return false;
        }

        // With zero-copy the engine writes the mapped host output via its device
        // alias, so the D2H copy is skipped entirely.
        if (!use_zero_copy_) {
            if (cudaMemcpyAsync(host_output_, device_output_, out_count * sizeof(float),
                                cudaMemcpyDeviceToHost, stream_) != cudaSuccess) {
                RCLCPP_ERROR(get_logger(), "D2H copy failed");
                return false;
            }
        }

        cudaError_t sync_err = cudaStreamSynchronize(stream_);
        if (sync_err != cudaSuccess) {
            RCLCPP_ERROR(get_logger(), "Stream sync failed: %s", cudaGetErrorString(sync_err));
            return false;
        }
        return true;
    }

    void publish_detections(const CameraContext& cam, const DetectionResult& det_result)
    {
        auto detection_array = std::make_unique<snappy_cpp::msg::DetectionArray>();
        detection_array->header.stamp = det_result.timestamp;
        detection_array->header.frame_id = cam.ns;
        detection_array->inference_time_ms = static_cast<uint32_t>(det_result.inference_time_ms);

        for (const auto& detection : det_result.detections) {
            snappy_cpp::msg::ObjectDetection obj_det;
            obj_det.object_class = CLASS_NAMES[detection.class_id];
            obj_det.confidence = detection.conf;
            obj_det.distance_m = detection.distance_m;
            obj_det.bounding_box.x = detection.box.x;
            obj_det.bounding_box.y = detection.box.y;
            obj_det.bounding_box.width = detection.box.width;
            obj_det.bounding_box.height = detection.box.height;
            obj_det.timestamp.sec = det_result.timestamp.seconds();
            obj_det.timestamp.nanosec = det_result.timestamp.nanoseconds() % 1000000000;
            detection_array->detections.push_back(obj_det);
        }

        cam.pub->publish(*detection_array);

        if (!detection_array->detections.empty()) {
            RCLCPP_DEBUG_THROTTLE(get_logger(), *this->get_clock(), 5000,
                "[%s] published %zu detections (inference: %.1fms)",
                cam.ns.c_str(), detection_array->detections.size(), det_result.inference_time_ms);
        }
    }

    void render_display(const CameraContext& cam, const cv::Mat& color_image,
                        const DetectionResult& det_result)
    {
        cv::Mat annotated = color_image.clone();
        for (const auto& d : det_result.detections) {
            cv::Rect rect((int)d.box.x, (int)d.box.y, (int)d.box.width, (int)d.box.height);
            cv::rectangle(annotated, rect, cv::Scalar(0, 255, 0), 2);

            std::string distance_str = (d.distance_m > 0) ?
                std::to_string((int)(d.distance_m * 100)) + "cm" : "N/A";
            std::string label = CLASS_NAMES[d.class_id] + " " +
                                std::to_string((int)(d.conf * 100)) + "% [" + distance_str + "]";

            int baseLine = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseLine);
            int textX = (int)d.box.x;
            int textY = (int)d.box.y - 5;
            cv::rectangle(annotated,
                cv::Point(textX, textY - textSize.height - baseLine - 4),
                cv::Point(textX + textSize.width + 4, textY + baseLine),
                cv::Scalar(0, 255, 0), cv::FILLED);
            cv::putText(annotated, label, cv::Point(textX + 2, textY - baseLine - 2),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
        }

        std::string info = "Infer: " + std::to_string((int)det_result.inference_time_ms) +
                           "ms (GPU)  det: " + std::to_string(det_result.detections.size());
        cv::putText(annotated, info, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        cv::imshow(cam.display_window_name, annotated);
        cv::moveWindow(cam.display_window_name, static_cast<int>(cam.index) * 680, 0);
        if (cv::waitKey(1) == 27) {
            RCLCPP_INFO(get_logger(), "ESC pressed, shutting down");
            rclcpp::shutdown();
        }
    }

    // --- Configuration ----------------------------------------------------
    std::vector<std::string> camera_namespaces_;
    double inference_hz_ = 10.0;
    bool display_ = false;
    bool save_images_ = true;
    std::string save_dir_;
    int distance_samples_ = 100;
    int distance_grid_ = 10;
    int max_queue_depth_ = 2;
    int requested_max_batch_ = 1;
    std::chrono::duration<double, std::milli> batch_collect_{0.0};

    std::vector<std::unique_ptr<CameraContext>> cameras_;

    // --- Inference job queue + worker ------------------------------------
    std::deque<InferenceJob> job_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    std::atomic<bool> stop_{false};
    std::atomic<uint64_t> dropped_jobs_{0};
    std::atomic<uint64_t> saved_images_{0};

    // --- Shared TensorRT / CUDA pipeline ---------------------------------
    TRTLogger trt_logger_;
    nvinfer1::IRuntime* runtime_ = nullptr;
    nvinfer1::ICudaEngine* engine_ = nullptr;
    nvinfer1::IExecutionContext* context_ = nullptr;

    bool use_zero_copy_ = false;
    float* host_input_ = nullptr;     // mapped backing for device_input_ (zero-copy only)
    float* host_output_ = nullptr;    // mapped (zero-copy) or pinned D2H staging
    float* device_input_ = nullptr;   // engine input (written by the preprocess kernel)
    float* device_output_ = nullptr;  // engine output
    cudaStream_t stream_ = nullptr;

    // Reusable device buffer holding the raw BGR source image for the kernel.
    uint8_t* d_src_ = nullptr;
    size_t src_capacity_ = 0;

    std::string input_name_;
    std::string output_name_;
    int input_w_ = 640;
    int input_h_ = 640;

    // Batch configuration resolved from the engine at startup.
    bool dynamic_engine_ = false;     // engine has a dynamic batch axis
    int engine_max_batch_ = 1;        // max batch the engine actually supports
    int effective_max_batch_ = 1;     // min(requested, engine_max), >= 1
    size_t single_input_count_ = 0;   // float elements per sample (3*H*W)
    size_t single_output_count_ = 0;  // float elements per sample (e.g. 300*6)
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CameraInferenceNode>();

    // MultiThreadedExecutor lets the per-camera (mutually-exclusive) callback
    // groups enqueue concurrently; the actual GPU work is on the worker thread.
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node);
    executor.spin();

    rclcpp::shutdown();
    return 0;
}
