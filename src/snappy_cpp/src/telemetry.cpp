 #include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float32.hpp"
#include "geometry_msgs/msg/vector3_stamped.hpp"
#include "snappy_cpp/msg/pose.hpp"
#include "snappy_cpp/msg/detection_array.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class TelemetryNode : public rclcpp::Node
{
public:
    TelemetryNode() : Node("telemetry_node")
    {
        RCLCPP_INFO(this->get_logger(), "Initializing Telemetry Node...");

        // Initialize CSV file stream
        csv_file_.open("sub_telemetry_log.csv", std::ios::out | std::ios::app);
        if (csv_file_.is_open()) {
            // Write CSV headers if file is empty/new
            csv_file_ << "timestamp_ns,depth,pos_x,pos_y,pos_z,roll,pitch,yaw,front_detections,bottom_detections\n";
        } else {
            RCLCPP_ERROR(this->get_logger(), "Failed to open CSV log file!");
        }

        // --- Subscriptions Setup ---

        // 1. Depth sensor subscription
        depth_sub_ = this->create_subscription<std_msgs::msg::Float32>(
            "depth_data", 10, std::bind(&TelemetryNode::depth_callback, this, _1));

        // 2. State Estimator Pose subscription
        state_sub_ = this->create_subscription<snappy_cpp::msg::Pose>(
            "state_estimator/state", 10, std::bind(&TelemetryNode::state_callback, this, _1));

        // 3. IMU Euler Angles subscription
        euler_sub_ = this->create_subscription<geometry_msgs::msg::Vector3Stamped>(
            "/filter/euler", 10, std::bind(&TelemetryNode::euler_callback, this, _1));

        // 4. Motor Command subscription (Must match Best Effort QoS from micro-ROS/controller)
        motor_sub_ = this->create_subscription<snappy_interfaces::msg::ThrusterCommand>(
            "/motor_cmd", rclcpp::QoS(10).best_effort(), std::bind(&TelemetryNode::motor_callback, this, _1));

        // 5. Camera Detections subscriptions (Must match SensorDataQoS from camera_inference)
        front_cam_sub_ = this->create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d455/detections", rclcpp::SensorDataQoS(), std::bind(&TelemetryNode::front_cam_callback, this, _1));

        bottom_cam_sub_ = this->create_subscription<snappy_cpp::msg::DetectionArray>(
            "/d405/detections", rclcpp::SensorDataQoS(), std::bind(&TelemetryNode::bottom_cam_callback, this, _1));

        // --- Timers Setup ---
        // Display loop timer running at 5Hz (every 200ms)
        display_timer_ = this->create_wall_timer(
            200ms, std::bind(&TelemetryNode::display_telemetry, this));

        RCLCPP_INFO(this->get_logger(), "Telemetry Node successfully running and logging.");
    }

    ~TelemetryNode() override
    {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
    }

private:
    // --- Callback Methods ---

    void depth_callback(const std_msgs::msg::Float32::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        current_depth_ = msg->data;
        log_to_csv();
    }

    void state_callback(const snappy_cpp::msg::Pose::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        pos_x_ = msg->position.x;
        pos_y_ = msg->position.y;
        pos_z_ = msg->position.z;
        log_to_csv();
    }

    void euler_callback(const geometry_msgs::msg::Vector3Stamped::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        roll_ = msg->vector.x;
        pitch_ = msg->vector.y;
        yaw_ = msg->vector.z;
        log_to_csv();
    }

    void motor_callback(const snappy_interfaces::msg::ThrusterCommand::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        // Storing active state or logging commands dynamically can go here if needed
        (void)msg; 
    }

    void front_cam_callback(const snappy_cpp::msg::DetectionArray::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        front_detections_.clear();
        for (const auto& det : msg->detections) {
            front_detections_.push_back(det.object_class + " (" + std::to_string((int)(det.confidence * 100)) + "%)");
        }
    }

    void bottom_cam_callback(const snappy_cpp::msg::DetectionArray::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        bottom_detections_.clear();
        for (const auto& det : msg->detections) {
            bottom_detections_.push_back(det.object_class + " (" + std::to_string((int)(det.confidence * 100)) + "%)");
        }
    }

    // --- Output Operations ---

    void log_to_csv() {
        if (!csv_file_.is_open()) return;

        uint64_t timestamp = this->get_clock()->now().nanoseconds();
        csv_file_ << timestamp << ","
                  << current_depth_ << ","
                  << pos_x_ << "," << pos_y_ << "," << pos_z_ << ","
                  << roll_ << "," << pitch_ << "," << yaw_ << ","
                  << front_detections_.size() << ","
                  << bottom_detections_.size() << "\n";
    }

    void display_telemetry()
    {
        std::lock_guard<std::mutex> lock(data_mutex_);

        // ANSI Escape code to clear terminal screen cleanly for visual output
        std::cout << "\033[2J\033[1;1H"; 
        std::cout << "====================================================\n";
        std::cout << "                  TELEMETRY SYSTEM                  \n";
        std::cout << "====================================================\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << " [ENVIRONMENT]\n";
        std::cout << "   Depth Sensor : " << current_depth_ << " m\n\n";
        
        std::cout << " [STATE ESTIMATION]\n";
        std::cout << "   Position     : X: " << pos_x_ << " | Y: " << pos_y_ << " | Z: " << pos_z_ << "\n";
        std::cout << "   Orientation  : R: " << roll__  << " | P: " << pitch_ << " | Y: " << yaw_   << "\n\n";

        std::cout << " [COMPUTER VISION DETECTION]\n";
        std::cout << "   Front Camera (D455)  : ";
        if (front_detections_.empty()) std::cout << "None\n";
        else {
            for (const auto& d : front_detections_) std::cout << "[" << d << "] ";
            std::cout << "\n";
        }

        std::cout << "   Bottom Camera (D405) : ";
        if (bottom_detections_.empty()) std::cout << "None\n";
        else {
            for (const auto& d : bottom_detections_) std::cout << "[" << d << "] ";
            std::cout << "\n";
        }
        std::cout << "====================================================\n";
    }

    // --- ROS Subscriptions & Resources ---
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr depth_sub_;
    rclcpp::Subscription<snappy_cpp::msg::Pose>::SharedPtr state_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr euler_sub_;
    rclcpp::Subscription<snappy_interfaces::msg::ThrusterCommand>::SharedPtr motor_sub_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr front_cam_sub_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr bottom_cam_sub_;
    rclcpp::TimerBase::SharedPtr display_timer_;

    // --- Telemetry Cache & Synchronization ---
    std::mutex data_mutex_;
    std::ofstream csv_file_;

    float current_depth_ = 0.0f;
    float pos_x_ = 0.0f, pos_y_ = 0.0f, pos_z_ = 0.0f;
    float roll_ = 0.0f, pitch_ = 0.0f, yaw_ = 0.0f;
    std::vector<std::string> front_detections_;
    std::vector<std::string> bottom_detections_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    std::make_shared<TelemetryNode>();
    rclcpp::spin(std::make_shared<TelemetryNode>());
    rclcpp::shutdown();
    return 0;
}
