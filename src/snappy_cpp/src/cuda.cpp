#include <opencv2/core/check.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/utility.hpp>
#include <rclcpp/logging.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>
#include <NvInferRuntime.h>
#include <cuda_runtime.h>

class CudaNode : public rclcpp::Node
{
public:
    CudaNode() : Node("cuda_node")
    {
        // temporary placeholder
        RCLCPP_INFO(get_logger(), "=== CUDA + Tensor RT Check ===");
        checkCUDA();
        checkTensorRT();
    }


    private:
    class TRTLogger : public nvinfer1::ILogger {
        public:
            rclcpp::Logger ros_logger;
            TRTLogger(rclcpp::Logger logger) : ros_logger(logger) {}
            void log(Severity severity, const char* msg) noexcept override {
                if (severity < Severity::kWARNING) {
                    RCLCPP_WARN(ros_logger, "[TRT] %s", msg);
                }
            }
    };

    void checkCUDA() {
        int deviceCount;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess) {
            RCLCPP_ERROR(this->get_logger(), "Failed to get device count: %s", cudaGetErrorString(err));
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Found %d CUDA devices", deviceCount);

        if (deviceCount == 0) {
            RCLCPP_ERROR(this->get_logger(), "No CUDA devices found ❌");
            return;
        }
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        RCLCPP_INFO(get_logger(), "GPU: %s", prop.name);
        RCLCPP_INFO(get_logger(), "Compute capability: %d.%d", prop.major, prop.minor);
        RCLCPP_INFO(get_logger(), "GPU Memory: %zu MB", prop.totalGlobalMem / (1024 * 1024));
        RCLCPP_INFO(get_logger(), "CUDA: OK ✅");
    }

    void checkTensorRT() {
        TRTLogger trt_logger(get_logger());
        auto runtime = nvinfer1::createInferRuntime(trt_logger);

        if (!runtime) {
            RCLCPP_ERROR(get_logger(), "TensorRT runtime creation FAILED ❌");
            return;
        }

        RCLCPP_INFO(get_logger(), "TensorRT version: %d.%d.%d",
            NV_TENSORRT_MAJOR,
            NV_TENSORRT_MINOR,
            NV_TENSORRT_PATCH);
        RCLCPP_INFO(get_logger(), "TensorRT runtime: OK ✅");

        delete runtime;
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CudaNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
