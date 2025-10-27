// this file is going to contain the code for the state estimator
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>

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
        
        imu1_file.open("imu1.csv", std::fstream::app);
        imu2_file.open("imu2.csv", std::fstream::app);
        
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
        
        // From on-board IMU
        imu_sub2_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/imu/data",
            qos,
            std::bind(&StateEstimator::imu2_callback, this, std::placeholders::_1));
        
        /*
        // Alternative: separate gyro topic
        gyro_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/camera/camera/gyro/sample",
            qos,
            std::bind(&StateEstimator::gyro_callback, this, std::placeholders::_1));
        
        // Alternative: separate accel topic
        accel_sub_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/camera/camera/accel/sample",
            qos,
            std::bind(&StateEstimator::accel_callback, this, std::placeholders::_1));
        */
            
        RCLCPP_INFO(this->get_logger(), "State Estimator subscribed to:");
        RCLCPP_INFO(this->get_logger(), "  - /camera/camera/imu");
        RCLCPP_INFO(this->get_logger(), "  - /camera/camera/gyro/sample");
        RCLCPP_INFO(this->get_logger(), "  - /camera/camera/accel/sample");
        RCLCPP_INFO(this->get_logger(), "Waiting for IMU data...");
        
        // Timer to check status
        check_timer_ = this->create_wall_timer(
            std::chrono::seconds(2),
            std::bind(&StateEstimator::check_status, this));
    }

private:
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
    
    void imu2_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if (!imu_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First IMU message received!");
            imu_received_ = true;
        }
        imu_count_++;
        
        // Print combined IMU data every 20 messages
        if (imu_count_ % 20 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[IMU 2] Orient: [%.3f, %.3f, %.3f, %.3f] rad | Accel: [%.3f, %.3f, %.3f] m/s²",
                msg->orientation.x, msg->orientation.y, msg->orientation.z, msg->orientation.w,
                msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
        }
        
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
    //rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr gyro_sub_;
    //rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr accel_sub_;
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
