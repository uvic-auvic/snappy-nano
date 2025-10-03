// computer_vision_basic.cpp
//
// Minimal ROS2 node: Publishes RealSense IMU data (gyro, accel), shows camera feeds.
// Skeleton code for ONNX vision inference (commented).
//
// Dependencies: ROS2, OpenCV, librealsense2, std_msgs
// Build with ament_cmake or colcon, link rclcpp, std_msgs, opencv, realsense2

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>

using namespace std::chrono_literals;
using json = nlohmann::json;

class ComputerVisionBasic : public rclcpp::Node
{
public:
    ComputerVisionBasic() : Node("computer_vision_basic")
    {
        gyro_pub_ = this->create_publisher<std_msgs::msg::String>("gyro_data", 10);
        accel_pub_ = this->create_publisher<std_msgs::msg::String>("accel_data", 10);

        // RealSense config
        config_.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 15);
        config_.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 15);
        config_.enable_stream(RS2_STREAM_GYRO);
        config_.enable_stream(RS2_STREAM_ACCEL);

        pipe_.start(config_);
        align_to_color_ = std::make_unique<rs2::align>(RS2_STREAM_COLOR);

        timer_ = this->create_wall_timer(
            100ms, std::bind(&ComputerVisionBasic::timer_callback, this));
    }

    ComputerVisionBasic()
    {
        pipe_.stop();
        cv::destroyAllWindows();
    }

private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr gyro_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr accel_pub_;

    rs2::pipeline pipe_;
    rs2::config config_;
    std::unique_ptr<rs2::align> align_to_color_;

    void timer_callback()
    {
        rs2::frameset frames;
        try {
            frames = pipe_.wait_for_frames(1000);
        } catch (const rs2::error& e) {
            RCLCPP_WARN(this->get_logger(), "RealSense frame wait failed: %s", e.what());
            return;
        }

        // Align depth to color
        auto aligned_frames = align_to_color_->process(frames);
        rs2::video_frame color_frame = aligned_frames.get_color_frame();
        rs2::depth_frame depth_frame = aligned_frames.get_depth_frame();

        // Show color image
        if (color_frame) {
            cv::Mat color_img(cv::Size(640, 480), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
            cv::imshow("RealSense Color", color_img);
        }

        // Show depth image (as grayscale)
        if (depth_frame) {
            cv::Mat depth_img(cv::Size(640, 480), CV_16U, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
            cv::Mat depth_vis;
            depth_img.convertTo(depth_vis, CV_8U, 15.0 / 256); // scale for visibility
            cv::imshow("RealSense Depth", depth_vis);
        }

        // Show both windows and allow closing with 'q'
        int key = cv::waitKey(1);
        if ((key & 0xFF) == 'q') {
            rclcpp::shutdown();
        }

        // IMU: Publish gyro and accel data as JSON
        publish_imu(frames);
        
        // -------- Skeleton for ONNX inference --------
        /*
        // cv::Mat input_image = ... // acquire from color_frame
        // 1. Preprocess input_image (resize, normalize, etc.)
        // 2. Create ONNX Runtime session, load model (see main example)
        // 3. Run inference
        // 4. Postprocess results, draw bounding boxes, etc.
        */
    }

    void publish_imu(const rs2::frameset& frames)
    {
        // Gyro
        auto gyro_frame = frames.first_or_default(RS2_STREAM_GYRO);
        if (gyro_frame) {
            rs2_vector gyro = gyro_frame.as<rs2::motion_frame>().get_motion_data();
            json gyro_json = {{"gyro", {{"x", gyro.x}, {"y", gyro.y}, {"z", gyro.z}}}};
            std_msgs::msg::String gyro_msg;
            gyro_msg.data = gyro_json.dump();
            gyro_pub_->publish(gyro_msg);
        }
        // Accel
        auto accel_frame = frames.first_or_default(RS2_STREAM_ACCEL);
        if (accel_frame) {
            rs2_vector accel = accel_frame.as<rs2::motion_frame>().get_motion_data();
            json accel_json = {{"accel", {{"x", accel.x}, {"y", accel.y}, {"z", accel.z}}}};
            std_msgs::msg::String accel_msg;
            accel_msg.data = accel_json.dump();
            accel_pub_->publish(accel_msg);
        }
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ComputerVisionBasic>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
