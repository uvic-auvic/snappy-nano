cmake_minimum_required(VERSION 3.5)
project(snappy_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(helloWorld src/test.cpp)
ament_target_dependencies(helloWorld rclcpp std_msgs)

add_executable(pressureSensor src/pressure_sensor.cpp)
ament_target_dependencies(pressureSensor rclcpp std_msgs)

install(TARGETS
  helloWorld
  pressureSensor
  DESTINATION lib/${PROJECT_NAME})

ament_package()
