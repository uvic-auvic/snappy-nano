// this file will have all the controller logic for the submarine
// Motor command publisher — publishes Float32MultiArray on /motor_cmd
// data[0] = MotorSelect (0–255, cast to uint8 on the STM32 side)
// data[1] = Speed       (–100.0 to 100.0)

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>

using namespace std::chrono_literals;

class Controller : public rclcpp::Node
{
public:
    Controller() : Node("controller")
    {
        pub_ = create_publisher<std_msgs::msg::Float32MultiArray>("/motor_cmd", 10);
        timer_ = create_wall_timer(100ms, std::bind(&Controller::publish_cmd, this));

        // Declare parameters so they can be set from the command line or launch file
        this->declare_parameter("motor_select", 255.0);  // default: all 8 motors
        this->declare_parameter("speed", 0.0);            // default: stopped

        RCLCPP_INFO(get_logger(), "Controller started — publishing on /motor_cmd at 10 Hz");
    }

private:
    void publish_cmd()
    {
        double motor_select = this->get_parameter("motor_select").as_double();
        double speed = this->get_parameter("speed").as_double();

        auto msg = std_msgs::msg::Float32MultiArray();
        msg.data = {static_cast<float>(motor_select), static_cast<float>(speed)};
        pub_->publish(msg);

        RCLCPP_INFO(get_logger(), "Publishing: motors=%.0f speed=%.1f", motor_select, speed);
    }

    rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Controller>());
    rclcpp::shutdown();
    return 0;
}
