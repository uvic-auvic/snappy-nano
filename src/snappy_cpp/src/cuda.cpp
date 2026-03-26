#include <opencv2/core/check.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/utility.hpp>
#include <rclcpp/logging.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/detail/image__struct.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <vision_msgs/msg/detection2d_array.hpp>
#include <cuda_runtime_api.h>
#include <chrono>
#include <NvInferRuntime.h>
#include <cuda_runtime.h>


#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

using namespace std::chrono_literals;

class CudaNode : public rclcpp::Node
{
public:
    CudaNode() : Node("cuda_node")
    {
        // temporary placeholder
        RCLCPP_INFO(get_logger(), "=== CUDA + Tensor RT Check ===");
        checkCUDA();
        checkTensorRT();

        //get camera feed from
        colorCameraFeed_ = this->create_subscription<sensor_msgs::msg::Image>(
            "camera/color/image_raw",
            10,
            std::bind(&CudaNode::color_callback, this, std::placeholders::_1)
        );


    }
    ~CudaNode();


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
    void color_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
        try {
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
            cv::Mat color_image = cv_ptr->image;

            // Display or process image here
            cv::imshow("RealSense Color", color_image);
            cv::waitKey(1);

        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr image_sub_;
    rclcpp::Publisher<vision_msgs::msg::Detection2DArray>::SharedPtr det_pub_;

    void loadEngine(const std::string& engine_path);
    std::vector<Detection> runInference(cv::cuda::GpuMat& frame);

    nvinfer1::IRuntime* runtime_;
    nvinfer1::ICudaEngine* engine_;
    nvinfer1::IExecutionContext* context_;


    void* gpu_buffers_[2];
    float* cpu_output_;
    cudaStream_t stream_;

    const int FRAMEHW = 640;
    const int NUM_CLASSES = 80;
    const float CONF_THRESHOLD = 0.5f;
    const float NMS_THRESHOLD = 0.45f;
};


int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CudaNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
