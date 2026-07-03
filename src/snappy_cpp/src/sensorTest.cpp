#include <cstdint>
#include <cmath>
#include <optional>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "std_msgs/msg/float32.hpp"
#include "snappy_cpp/msg/pose.hpp"
#include "snappy_interfaces/msg/thruster_command.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <queue>

using namespace std::chrono_literals;
using std::placeholders::_1;

struct SensorEntry {
  std::string name;
  std::string topic;
  std::string msg_type;
};

class SensorTest : public rclcpp::Node
{
public:
  SensorTest() : Node("sensor_test")
  {
    std::vector<SensorEntry> sensors = {
      // --- Cameras ---
      {"D455 Color",          "/d455/color/image_raw",              "sensor_msgs/msg/Image"},
      {"D455 Depth",          "/d455/depth/image_rect_raw",         "sensor_msgs/msg/Image"},
      {"D455 IMU",            "/d455/imu",                          "sensor_msgs/msg/Imu"},
      {"D405 Color",          "/d405/color/image_raw",              "sensor_msgs/msg/Image"},
      {"D405 Depth",          "/d405/depth/image_rect_raw",         "sensor_msgs/msg/Image"},
      // --- IMU (Xsens) ---
      {"Xsens Euler",         "/filter/euler",                      "geometry_msgs/msg/Vector3Stamped"},
      // --- Pressure / Depth ---
      {"Pressure Depth",      "/depth_data",                        "std_msgs/msg/Float32"},
      // --- DVL ---
      {"DVL Odometry",        "/waterlinked_dvl_driver/odom",       "nav_msgs/msg/Odometry"},
      // --- Motor command echo (confirms motorboard is live) ---
      {"Motor Command",       "/motor_cmd",                         "snappy_interfaces/msg/ThrusterCommand"},
    };

    timeout_sec_ = 4.0;  // pressure node has a 3s delay; motors may take longer

    for (auto & s : sensors) {
      received_[s.name] = false;

      auto sub = this->create_generic_subscription(
        s.topic, s.msg_type, rclcpp::QoS(10).best_effort(),
        [this, name = s.name](std::shared_ptr<rclcpp::SerializedMessage>) {
          if (!received_[name]) {
            RCLCPP_INFO(this->get_logger(), "  [PASS] %-25s is publishing.", name.c_str());
            received_[name] = true;
            check_if_done();
          }
        });
      subs_.push_back(sub);
    }

    // Fallback: print results after hard timeout regardless
    timer_ = this->create_wall_timer(
      std::chrono::duration<double>(timeout_sec_),
      [this]() {
        timer_->cancel();
        print_results();
        rclcpp::shutdown();
      });

    RCLCPP_INFO(this->get_logger(),
      "SensorTest: waiting up to %.1f seconds for all sensors...\n", timeout_sec_);
  }

private:
  void check_if_done()
  {
    for (auto & [name, ok] : received_) {
      if (!ok) return;  // still waiting on something
    }
    // All sensors received at least one message — no need to wait for timeout
    timer_->cancel();
    print_results();
    rclcpp::shutdown();
  }

  void print_results()
  {
    bool all_passed = true;
    int passed = 0;

    RCLCPP_INFO(this->get_logger(), "\n========== SENSOR TEST RESULTS ==========");
    for (auto & [name, ok] : received_) {
      if (ok) {
        RCLCPP_INFO(this->get_logger(),  "  [PASS] %s", name.c_str());
        passed++;
      } else {
        RCLCPP_ERROR(this->get_logger(), "  [FAIL] %s — no messages received", name.c_str());
        all_passed = false;
      }
    }
    RCLCPP_INFO(this->get_logger(),
      "=========================================");
    RCLCPP_INFO(this->get_logger(),
      "  Result: %d / %zu sensors OK", passed, received_.size());
    RCLCPP_INFO(this->get_logger(),
      "=========================================");

    if (all_passed) {
      RCLCPP_INFO(this->get_logger(),  "  ALL SENSORS OK — Snappy is ready to dive.");
    } else {
      RCLCPP_ERROR(this->get_logger(), "  SENSOR FAILURE — Do not operate the submarine.");
    }
  }

  std::map<std::string, bool> received_;
  std::vector<rclcpp::GenericSubscription::SharedPtr> subs_;
  rclcpp::TimerBase::SharedPtr timer_;
  double timeout_sec_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SensorTest>());
  rclcpp::shutdown();
  return 0;
}
