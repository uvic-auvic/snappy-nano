// State estimator node. Subscribes to two IMU streams — the RealSense camera's
// fused IMU (/camera/camera/imu) and the on-board IMU (/imu) — logs both to CSV
// for offline analysis, and republishes the on-board IMU's orientation as a
// Pose on state_estimator/state for the controller. This is currently a
// pass-through of the on-board orientation; no real fusion is done yet.
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <system_error>
#include "snappy_cpp/msg/pose.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

using namespace std::chrono_literals;

class StateEstimator : public rclcpp::Node
{
public:

    std::fstream imu1_file;
    std::fstream imu2_file;

    StateEstimator() : Node("state_estimator")
    {
        RCLCPP_INFO(this->get_logger(), "State Estimator starting...");
        
        // imu1_file: accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z
        // imu2_file: accel_x, accel_y, accel_z, quat_x, quat_y, quat_z, quat_w
        
        // Write IMU CSV logs into a (gitignored) logs/ directory so they don't
        // clutter the working directory. The directory is configurable via the
        // "log_dir" parameter and created if it doesn't exist.
        const std::string log_dir = this->declare_parameter<std::string>("log_dir", "logs");
        std::error_code ec;
        std::filesystem::create_directories(log_dir, ec);
        const std::filesystem::path dir = ec ? std::filesystem::path(".")
                                             : std::filesystem::path(log_dir);
        if (ec) {
            RCLCPP_WARN(this->get_logger(),
                "Could not create log dir '%s' (%s); writing to working directory.",
                log_dir.c_str(), ec.message().c_str());
        }

        imu1_file.open((dir / "imu1.csv").string(), std::fstream::app);
        imu2_file.open((dir / "imu2.csv").string(), std::fstream::app);
        RCLCPP_INFO(this->get_logger(), "Logging IMU data to %s/{imu1,imu2}.csv",
            dir.string().c_str());
        
        // Create QoS profile matching RealSense camera publisher
        // RealSense uses: Best Effort reliability + Volatile durability
        auto qos = rclcpp::QoS(rclcpp::KeepLast(10))
            .best_effort()  // Use Best Effort (not Reliable)
            .durability_volatile();  // Volatile durability
        
        // Primary: unified IMU topic (gyro + accel combined) from camera
        imu_sub1_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/camera/camera/imu",
            qos,
            std::bind(&StateEstimator::imu1_callback, this, std::placeholders::_1));
        
        // From on-board IMU; this is the stream we republish as the state Pose.
        imu_sub2_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/imu", //"/imu/data"
            qos,
            std::bind(&StateEstimator::imu2_callback, this, std::placeholders::_1));
        publisher_ = this->create_publisher<snappy_cpp::msg::Pose>(
            "state_estimator/state", 10
        );

        RCLCPP_INFO(this->get_logger(), "State Estimator subscribed to:");
        RCLCPP_INFO(this->get_logger(), "  - /camera/camera/imu");
        RCLCPP_INFO(this->get_logger(), "  - /imu");
        RCLCPP_INFO(this->get_logger(), "Waiting for IMU data...");
        
        // Timer to check status
        check_timer_ = this->create_wall_timer(
            std::chrono::seconds(2),
            std::bind(&StateEstimator::check_status, this));
    }

private:
    // RealSense fused IMU: log gyro + accel to imu1.csv and periodically print.
    // Not republished — only the on-board IMU drives the state Pose.
    void imu1_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if (!imu_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First IMU message received!");
            imu_received_ = true;
        }
        imu_count_++;
        
        // Print combined IMU data every 20 messages
        if (imu_count_ % 20 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[IMU 1] Gyro: [%.3f, %.3f, %.3f] rad/s | Accel: [%.3f, %.3f, %.3f] m/s²",
                msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z,
                msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
        }
        
        // Write all data to file
        imu1_file << msg->linear_acceleration.x << ","
                << msg->linear_acceleration.y << ","
                << msg->linear_acceleration.z << ","
            << msg->angular_velocity.x << ","
                << msg->angular_velocity.y << ","
                << msg->angular_velocity.z << ","
                << std::endl;
    }
    
    // On-board IMU: republish its orientation as the state Pose, and log to
    // imu2.csv. NOTE: position is filled with the orientation's x/y/z here as a
    // placeholder — there is no real position estimate yet.
    void imu2_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        auto pose = snappy_cpp::msg::Pose();

        if (!imu_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First IMU message received!");
            imu_received_ = true;
        }
        imu_count_++;
        pose.position.x = msg->orientation.x;
        pose.position.y = msg->orientation.y;
        pose.position.z = msg->orientation.z;

        pose.orientation.x = msg->orientation.x;
        pose.orientation.y = msg->orientation.y;
        pose.orientation.z = msg->orientation.z;
        pose.orientation.w = msg->orientation.w;
        // Print combined IMU data every 20 messages
        if (imu_count_ % 20 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[IMU 2] Orient: [%.3f, %.3f, %.3f, %.3f] rad | Accel: [%.3f, %.3f, %.3f] m/s²",
                msg->orientation.x, msg->orientation.y, msg->orientation.z, msg->orientation.w,
                msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
        }   
        publisher_->publish(pose);
        // Write all data to file
        imu2_file << msg->linear_acceleration.x << ","
                << msg->linear_acceleration.y << ","
                << msg->linear_acceleration.z << ","
                << msg->orientation.x << ","
                << msg->orientation.y << ","
                << msg->orientation.z << ","
                << msg->orientation.w << ","
                << std::endl;
    }
    
    // Handlers for the RealSense's separate gyro/accel sample topics. Currently
    // inactive (no subscription is created); kept for when the split streams are
    // used instead of the fused /camera/camera/imu topic. To enable, add the
    // gyro_sub_/accel_sub_ members and wire them up in the constructor:
    //
    //   gyro_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
    //       "/camera/camera/gyro/sample", qos,
    //       std::bind(&StateEstimator::gyro_callback, this, std::placeholders::_1));
    //   accel_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
    //       "/camera/camera/accel/sample", qos,
    //       std::bind(&StateEstimator::accel_callback, this, std::placeholders::_1));
    void gyro_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if (!gyro_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First Gyro message received!");
            gyro_received_ = true;
        }
        gyro_count_++;
        
        if (gyro_count_ % 50 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[Gyro] X=%.3f, Y=%.3f, Z=%.3f rad/s (count: %d)",
                msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z, gyro_count_);
        }
    }
    
    void accel_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if (!accel_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First Accel message received!");
            accel_received_ = true;
        }
        accel_count_++;
        
        if (accel_count_ % 50 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[Accel] X=%.3f, Y=%.3f, Z=%.3f m/s² (count: %d)",
                msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z, accel_count_);
        }
    }
    
    // Every 2 s, warn if no IMU data has arrived yet, otherwise report counts.
    void check_status()
    {
        if (!imu_received_ && !gyro_received_ && !accel_received_) {
            RCLCPP_WARN(this->get_logger(), 
                "❌ No IMU data received yet. Total: IMU=%d, Gyro=%d, Accel=%d",
                imu_count_, gyro_count_, accel_count_);
        } else {
            RCLCPP_INFO(this->get_logger(), 
                "✅ Receiving - IMU:%d Gyro:%d Accel:%d msgs", 
                imu_count_, gyro_count_, accel_count_);
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub1_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub2_;
    rclcpp::Publisher<snappy_cpp::msg::Pose>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr check_timer_;
    
    bool imu_received_ = false;
    bool gyro_received_ = false;
    bool accel_received_ = false;
    int imu_count_ = 0;
    int gyro_count_ = 0;
    int accel_count_ = 0;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<StateEstimator>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
