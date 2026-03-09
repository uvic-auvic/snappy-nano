// computer_vision_basic.cpp
//
// Minimal ROS2 node: Subscribes to RealSense camera topics from realsense2_camera node
// Skeleton code for ONNX vision inference (commented).
//
// Dependencies: ROS2, OpenCV, sensor_msgs, cv_bridge
// Build with ament_cmake or colcon

#include <opencv2/videoio.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/msg/imu.hpp>
// #include <onnxruntime_cxx_api.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <chrono>

using namespace std::chrono_literals;
using namespace cv;

class ComputerVision : public rclcpp::Node
{
public:
    ComputerVision() : Node("computer_vision")
    {
        RCLCPP_INFO(this->get_logger(), "Computer Vision node started");

        // Subscribe to RealSense camera topics
        color_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/color/image_rect_raw", 10,
            std::bind(&ComputerVision::color_callback, this, std::placeholders::_1));

        depth_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/camera/camera/depth/image_rect_raw", 10,
            std::bind(&ComputerVision::depth_callback, this, std::placeholders::_1));

        imu_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/camera/camera/imu", 10,
            std::bind(&ComputerVision::imu_callback, this, std::placeholders::_1));

        timer_ = this->create_wall_timer(
            1s, std::bind(&ComputerVision::timer_callback, this));
    }

private:
    void timer_callback()
    {
        RCLCPP_INFO(this->get_logger(), "Computer Vision running...");
    }

    void color_callback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        try {
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, "bgr8");
            cv::Mat color_image = cv_ptr->image;
            std::cout << "test1" << std::endl;

            // Display or process image here
            cv::imshow("RealSense Color", color_image);
            //show framerate
            cv::putText(color_image, std::to_string(cv::getTickFrequency()), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0, 255, 0), 2);
            cv::waitKey(1);
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void depth_callback(const sensor_msgs::msg::Image::SharedPtr msg)
    {
        try {
            cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::TYPE_16UC1);
            cv::Mat depth_image = cv_ptr->image;

            // Normalize for visualization
            cv::Mat depth_display;
            depth_image.convertTo(depth_display, CV_8U, 255.0/10000.0);
            cv::imshow("RealSense Depth", depth_display);
            cv::waitKey(1);

        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
        }
    }

    void colorBreakDown() {
    	cv::Vec3b bgrPixel(40,158,16);
    	//create Mat object from vector
    	cv::Mat3b bgr (bgrPixel);

    	cv::Mat3b hsv, ycb, lab;
    	//ycb is
    	cv::cvtColor(bgr, ycb, cv::COLOR_BGR2YCrCb);
    	//hsv is
    	cv::cvtColor(bgr, hsv, cv::COLOR_BGR2YCrCb);
    	//lab is
    	cv::cvtColor(bgr, lab, cv::COLOR_BGR2YCrCb);

    	cv::Vec3b ycbPixel(ycb.at<cv::Vec3b>(0,0));
    	cv::Vec3b hsvPixel(hsv.at<cv::Vec3b>(0,0));
    	cv::Vec3b labPixel(lab.at<cv::Vec3b>(0,0));

    	int thresh = 55;

    	cv::Scalar minBGR = cv::Scalar(bgrPixel.val[0] - thresh, bgrPixel.val[1] - thresh, bgrPixel[2] - thresh);
    	cv::Scalar maxBGR = cv::Scalar(bgrPixel.val[0] + thresh, bgrPixel.val[1] + thresh, bgrPixel[2] + thresh);
    	std::cout << "minBGR: " << minBGR << ", maxBGR: " << maxBGR << std::endl;
    }

    static constexpr int INPUT_HW = 640;
    static constexpr float conf_thresh = 0.25f;
    static constexpr float NMS_THRESH = 0.45f;
    static constexpr int NUM_THREADS = 4;

    struct Detection {
        cv::Rect2f box;
        float conf;
        int class_id;
    };

    void run_ObjectDetectionNonCuda() {
        //run onnx model



    };

    cv::Mat create_Boxes(const cv::Mat& feed,
        int target_width,
        int target_height,
        float& scale_factor,
        int& pad_x,
        int& pad_y) {

    };



    void imu_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        msg->angular_velocity;
        msg->linear_acceleration;
        // Process IMU data here
        // msg->angular_velocity (gyro);
        // msg->linear_acceleration (accel);
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr color_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depth_sub_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ComputerVision>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
