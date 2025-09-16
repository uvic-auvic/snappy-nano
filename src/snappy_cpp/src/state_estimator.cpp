#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <iostream>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "custom/msg/pose_e.hpp"
#include "nlohmann/json.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

using namespace std::chrono_literals;
using json = nlohmann:json;

