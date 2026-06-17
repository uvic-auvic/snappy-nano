#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <memory>

class TestNode : public rclcpp::Node
{
public:
    TestNode() : Node("test_node")
    {
        RCLCPP_INFO(this->get_logger(), "Hello, world!");
        
        // Create subscription to pressure data
        pressure_sub_ = this->create_subscription<std_msgs::msg::String>(
            "pressure_data", 10, 
            std::bind(&TestNode::pressure_callback, this, std::placeholders::_1));
        
        RCLCPP_INFO(this->get_logger(), "Test node started, listening for pressure data");
    }

private:
    void pressure_callback(const std_msgs::msg::String::SharedPtr msg)
    {
        RCLCPP_INFO(this->get_logger(), "Pressure data received: %s", msg->data.c_str());
    }
    
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr pressure_sub_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TestNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
