#include <rclcpp/rclcpp.hpp>

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  RCLCPP_INFO(rclcpp::get_logger("test_node"), "Hello, world!");
  rclcpp::shutdown();
  return 0;
}
