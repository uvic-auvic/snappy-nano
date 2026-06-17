//this will be the planner/FSM for what step of competition the submarine is on at competion,
// it will have all the high level instructions on what it is supposed to do
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "snappy_cpp/msg/task.hpp"
#include "snappy_cpp/msg/pose.hpp"
#include "snappy_cpp/msg/detection_array.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class Planner : public rclcpp::Node
{
public:
    Planner() : Node("planner")
    {

        RCLCPP_INFO(this->get_logger(), "Planner node started");
        // Subscribe to detection messages
        detection_sub_ = this->create_subscription<snappy_cpp::msg::DetectionArray>(
            "/cuda_node/detections",
            rclcpp::SensorDataQoS(),
            std::bind(&Planner::detection_callback, this, std::placeholders::_1));
        RCLCPP_INFO(this->get_logger(), "Subscribed to /cuda_node/detections");


        timer_ = this->create_wall_timer(
            1s, std::bind(&Planner::timer_callback, this));
        // task_timer_ = this->create_wall_timer(
        //     1s, std::bind(&Planner::task_timer, this));
    }

private:
    void timer_callback()
    {
        // uncomment for debug
        // RCLCPP_INFO(this->get_logger(), "Planner running...");
    }

    void detection_callback(const snappy_cpp::msg::DetectionArray::SharedPtr msg)
    {
        if (msg->detections.empty()) {
            return;
        }

        RCLCPP_INFO(this->get_logger(), "Received %zu detections (inference: %ums)",
            msg->detections.size(), msg->inference_time_ms);

        for (const auto& detection : msg->detections) {
            RCLCPP_DEBUG(this->get_logger(),
                "  - Object: %s | Confidence: %.2f | Distance: %.2fm | Box: (%.0f, %.0f, %.0f, %.0f)",
                detection.object_class.c_str(),
                detection.confidence,
                detection.distance_m,
                detection.bounding_box.x,
                detection.bounding_box.y,
                detection.bounding_box.width,
                detection.bounding_box.height);
        }
    }

    void starter_task(){
        auto task_msg = snappy_cpp::msg::Task();

        task_msg.type = "move";
        task_msg.direction = "z";
        task_msg.magnitude = 1;
        task_msg.absolute = false;
        task_msg.overwrite = false;

    }

    rclcpp::Publisher<snappy_cpp::msg::Task>::SharedPtr task_publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Subscription<snappy_cpp::msg::DetectionArray>::SharedPtr detection_sub_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Planner>());
    rclcpp::shutdown();
    return 0;
}
