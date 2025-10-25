// this file will have all the controller logic for the submarine

#include <rclcpp/rclcpp.hpp>
#include <chrono>

using namespace std::chrono_literals;

class Controller : public rclcpp::Node
{
public:
    Controller() : Node("controller")
    {
        RCLCPP_INFO(this->get_logger(), "Controller node started");
        timer_ = this->create_wall_timer(
            1s, std::bind(&Controller::timer_callback, this));
    }

private:
    void timer_callback()
    {
        RCLCPP_INFO(this->get_logger(), "Controller running...");
    }
    
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Controller>());
    rclcpp::shutdown();
    return 0;
}
