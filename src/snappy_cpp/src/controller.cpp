// this file will have all the controller logic for the submarine
// Motor command publisher — publishes Float32MultiArray on /motor_cmd
// data[0] = MotorSelect (0–255, cast to uint8 on the STM32 side) 11111111 (255) = all 8 motors
// data[1] = Speed       (–100.0 to 100.0)

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>
#include <controller.h>

struct MotorCmd {
    uint8_t mask;
    float   speed;
};

using namespace std::chrono_literals;

class Controller : public rclcpp::Node
{
public:
    Controller() : Node("controller")
    {
        pub_ = create_publisher<std_msgs::msg::Float32MultiArray>("/motor_cmd", 10);
        timer_ = create_wall_timer(100ms, std::bind(&Controller::publish_cmd, this));
	flag_ = 0;

        // Declare parameters so they can be set from the command line or launch file
        //this->declare_parameter("motor_select", 255.0);  // default: all 8 motors
        //this->declare_parameter("speed", 0.0);            // default: stopped

	//auto msg = std_msgs::msg::Float32MultiArray();
	//RCLCPP_INFO(get_logger(), "On");
	//msg.data = {68.0, 20};
	//pub_->publish(msg);
	//rclcpp::sleep_for(std::chrono::seconds(1));
	//RCLCPP_INFO(get_logger(), "Off");
	//msg.data = {255.0, 0};
	//pub_->publish(msg);

        RCLCPP_INFO(get_logger(), "Controller started — publishing on /motor_cmd at 10 Hz");
    }
    // Single message — all motors same sign
    void forward    (float s) { sendCmd(MotorCalls::FORWARD,    s);  }
    void backward   (float s) { sendCmd(MotorCalls::FORWARD,   -s);  }
    void down       (float s) { sendCmd(MotorCalls::VERTICAL, -s); }
    void up    (float s) { sendCmd(MotorCalls::VERTICAL,  s); }

    // Dual message — opposing signs
    void strafeRight(float s) {
        sendDualCmd(
            {MotorCalls::FRONT_YAW,  s},
            {MotorCalls::BACK_YAW,   s}   // same sign = strafe
        );
    }
    void strafeLeft(float s) {
        sendDualCmd(
            {MotorCalls::FRONT_YAW, -s},
            {MotorCalls::BACK_YAW,  -s}   // same sign, both reversed
        );
        
    void yawRight(float s) {
        sendDualCmd(
            {MotorCalls::FRONT_YAW,  s},
            {MotorCalls::BACK_YAW,  -s}   // opposite sign = yaw
        );
    }
        
    void yawLeft(float s) {
        sendDualCmd(
            {MotorCalls::FRONT_YAW, -s},
            {MotorCalls::BACK_YAW,   s}
        );
    }
        
    void strafeLeft(float s) {
        sendDualCmd(
            {MotorCalls::FRONT_YAW, -s},
            {MotorCalls::BACK_YAW,  -s}   // same sign, both reversed
        );
    }
    
    void stop(){ sendCmd(MotorCalls::ALL, 0.0f); }
private:
    
    //this funciton I am leaving in the code base for testing microROS communication, the sendCmd function will be used for building out our movement system
    void publish_cmd()
    {
        //double motor_select = this->get_parameter("motor_select").as_double();
        //double speed = this->get_parameter("speed").as_double();

        auto msg = std_msgs::msg::Float32MultiArray();
	if (flag_ < 40) {
		msg.data = {1.0, 20.0 + static_cast<float>(flag_)};
		flag_++;
	} else {
		msg.data = {255.0, 0.0};
	}
        //msg.data = {static_cast<float>(motor_select), static_cast<float>(speed)};
        pub_->publish(msg);

        //RCLCPP_INFO(get_logger(), "Publishing: motors=%.0f speed=%.1f", motor_select, speed);
    }
    
    void sendCmd(uint8_t mask, float speed)
    {
        auto msg = std_msgs::msg::Float32MultiArray();
        msg.data = {static_cast<float>(mask), speed};
        pub_->publish(msg);
    }


    rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
    int flag_;
    
    
    void publish_dual_cmd(MotorCmd a, MotorCmd b) {
        sendCmd(a.mask, a.speed);
        sendCmd(b.mask, b.speed);
    }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Controller>());
    rclcpp::shutdown();
    return 0;
}
