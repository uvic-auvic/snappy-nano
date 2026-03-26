#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <chrono>

class CounterPublisher : public rclcpp::Node
{
public:
    CounterPublisher() : Node("counter_publisher"), count_(0)
    {
        pub_ = this->create_publisher<std_msgs::msg::Int32>("counter", 10);
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(500),
            [this]() {
                auto msg = std_msgs::msg::Int32();
                msg.data = count_++;
                pub_->publish(msg);
                RCLCPP_INFO(this->get_logger(), "Publishing: %d", msg.data);
            });
    }

private:
    rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<CounterPublisher>());
    rclcpp::shutdown();
    return 0;
}
