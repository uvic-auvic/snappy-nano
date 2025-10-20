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
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Build all executables
add_executable(helloWorld src/test.cpp)
ament_target_dependencies(helloWorld rclcpp std_msgs)

add_executable(pressureSensor src/pressure_sensor.cpp)
ament_target_dependencies(pressureSensor rclcpp std_msgs)

add_executable(computer_vision src/computer_vision.cpp)
ament_target_dependencies(computer_vision rclcpp std_msgs sensor_msgs geometry_msgs cv_bridge)
target_link_libraries(computer_vision ${OpenCV_LIBS})

add_executable(controller src/controller.cpp)
ament_target_dependencies(controller rclcpp std_msgs geometry_msgs)

add_executable(state_estimator src/state_estimator.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs)

# Install all executables
install(TARGETS
  helloWorld
  pressureSensor
  computer_vision
  controller
  state_estimator
  planner
  DESTINATION lib/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)

ament_package()
