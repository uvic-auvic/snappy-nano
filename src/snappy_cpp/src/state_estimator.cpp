// this file is going to contain the code for the state estimator
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"

#include "kalman.h"


using namespace std::chrono_literals;

class StateEstimator : public rclcpp::Node
{
public:

    std::fstream imu1_file;
    std::fstream imu2_file;
    // std::fstream depth_file;
    double last_time_imu1_sec_ = 0.0;
    double last_time_imu2_sec_ = 0.0;
    KalmanFilter kf;


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
        
        /*depth_sub_ = this->create_subscription<std_msgs::msg::String>(
            "pressure_data", 
            10,   
            std::bind(&StateEstimator::depth_callback, this, std::placeholders::_1));   */  

    

        // TODO: add uncertainty vaules, all set to 0 or 1 right now, need to tune based on sensor specs and expected noise levels
        MatrixXd P_init = MatrixXd::Identity(15, 15);
        P_init.block<3,3>(0,0)   *= 0.05;  // position uncertainty
        P_init.block<3,3>(3,3)   *= 0.1;   // velocity uncertainty
        P_init.block<3,3>(6,6)   *= 0.01;  // orientation error uncertainty
        P_init.block<3,3>(9,9)   *= 0.01;  // gyro bias uncertainty
        P_init.block<3,3>(12,12) *= 0.01;  // accel bias uncertainty

        kf.setInitialCovariance(P_init);

        MatrixXd Q_init = MatrixXd::Identity(12, 12);
        Q_init.block<3,3>(0,0) *= 0.1;    // accel noise
        Q_init.block<3,3>(3,3) *= 0.1;    // gyro noise
        Q_init.block<3,3>(6,6) *= 0.01;   // gyro bias random walk
        Q_init.block<3,3>(9,9) *= 0.01;   // accel bias random walk
        kf.setProcessNoise(Q_init);
        MatrixXd R_imu2_init = MatrixXd::Identity(3, 3) * 0.01;  // IMU2 orientation noise
        MatrixXd R_depth_init = MatrixXd::Identity(1, 1) * 0.05;  // depth sensor noise
        kf.setIMU2MeasurementNoise(R_imu2_init);
        kf.setDepthMeasurementNoise(R_depth_init);


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
            last_time_imu1_sec_ = rclcpp::Time(msg->header.stamp).seconds();
        }
        imu_count_++;

        const double now_sec = rclcpp::Time(msg->header.stamp).seconds();
        const double dt = now_sec - last_time_imu1_sec_;
        last_time_imu1_sec_ = now_sec;
        if(dt > 0.001) // Ensure dt is reasonable (not negative or huge)
        {
            VectorXd U(6);
            U << msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z,
                 msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z;
            kf.predict(U, dt); 
        }


        // Print combined IMU data every 20 messages
        if (imu_count_ % 20 == 0) {
            RCLCPP_INFO(this->get_logger(), 
                "[IMU 1] Gyro: [%.3f, %.3f, %.3f] rad/s | Accel: [%.3f, %.3f, %.3f] m/s²",
                msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z,
                msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
            RCLCPP_INFO(this->get_logger(), "  State Estimate: Pos=[%.2f, %.2f, %.2f] Vel=[%.2f, %.2f, %.2f] Ori=[%.2f, %.2f, %.2f]",
                kf.getPosition().x(), kf.getPosition().y(), kf.getPosition().z(),
                kf.getVelocity().x(), kf.getVelocity().y(), kf.getVelocity().z(),
                kf.getOrientation().x(), kf.getOrientation().y(), kf.getOrientation().z());
        }
        
        // Write all data to file
        imu1_file << msg->linear_acceleration.x << ","
                << msg->linear_acceleration.y << ","
                << msg->linear_acceleration.z << ","
                << msg->angular_velocity.x << ","
                << msg->angular_velocity.y << ","
                << msg->angular_velocity.z << std::endl;
    }
    
    void imu2_callback(const sensor_msgs::msg::Imu::SharedPtr msg)
    {
        if (!imu_received_) {
            RCLCPP_INFO(this->get_logger(), "✅ First IMU message received!");
            imu_received_ = true;
            last_time_imu2_sec_ = rclcpp::Time(msg->header.stamp).seconds();
        }
        imu_count_++;
        const double now_sec = rclcpp::Time(msg->header.stamp).seconds();
        const double dt = now_sec - last_time_imu2_sec_;
        if (dt > 0.001)
        {
            //Insure not first message, and that dt is reasonable (not negative or huge)
            kf.updateIMU2(Quaterniond(msg->orientation.w, msg->orientation.x, msg->orientation.y, msg->orientation.z));
        }
        last_time_imu2_sec_ = now_sec;
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
                << msg->orientation.w << std::endl;
    }

   /* void depth_callback(const std_msgs::msg::String::SharedPtr msg)
    {
        std::string depth_data = msg->data;
        RCLCPP_INFO(this->get_logger(), "Depth data received: %s", msg->data.c_str());

        // Write depth data to file
        depth_file << depth_data << std::endl;
    }*/
    
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
    //rclcpp::Subscription<std_msgs::msg::String>::SharedPtr depth_sub_;
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
