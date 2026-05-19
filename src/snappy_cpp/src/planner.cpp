//this will be the planner/FSM for what step of competition the submarine is on at competion, 
// it will have all the high level instructions on what it is supposed to do 
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "snappy_cpp/msg/task.hpp"
#include "snappy_cpp/msg/pose.hpp"

using namespace std::chrono_literals;
using std::placeholders::_1;

class Planner : public rclcpp::Node
{
public:
    Planner() : Node("planner")
    {

        RCLCPP_INFO(this->get_logger(), "Planner node started");

        //Publish task to Controller.
        task_publisher_ = this->create_publisher<snappy_cpp::msg::Task>("/planner/task",10);

        //Subscribe to controller status
        // status_subscription = this->create_subscription<std_msgs::msg::String>(
        //     "/controller/status",10, std::bind(&Planner::controller_callback, this, _1)
        // )

        timer_ = this->create_wall_timer(
            1s, std::bind(&Planner::timer_callback, this));
        // task_timer_ = this->create_wall_timer(
        //     1s, std::bind(&Planner::task_timer, this));
    }

private:
    void timer_callback()
    {  
        RCLCPP_INFO(this->get_logger(), "Planner running...");
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
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Planner>());
    rclcpp::shutdown();
    return 0;
}
