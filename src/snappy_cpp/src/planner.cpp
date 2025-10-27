//this will be the planner/FSM for what step of competition the submarine is on at competion, 
// it will have all the high level instructions on what it is supposed to do 

#include <rclcpp/rclcpp.hpp>
#include <chrono>

using namespace std::chrono_literals;

class Planner : public rclcpp::Node
{
public:
    Planner() : Node("planner")
    {
        RCLCPP_INFO(this->get_logger(), "Planner node started");
        timer_ = this->create_wall_timer(
            1s, std::bind(&Planner::timer_callback, this));
    }

private:
    void timer_callback()
    {
        RCLCPP_INFO(this->get_logger(), "Planner running...");
    }
    
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Planner>());
    rclcpp::shutdown();
    return 0;
}
