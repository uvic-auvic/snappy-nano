cmake_minimum_required(VERSION 3.5)
project(snappy_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${Boost_INCLUDE_DIRS})

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# Build all executables
set(BOOST_ROOT "/usr")
find_package(Boost REQUIRED)

add_library(serial_lib src/include/src/serial.cpp)
ament_target_dependencies(serial_lib rclcpp std_msgs)

add_executable(snappy_cpp src/test.cpp)
ament_target_dependencies(snappy_cpp rclcpp std_msgs)
target_link_libraries(snappy_cpp ${Boost_LIBRARIES})

add_executable(pressureSensor src/pressure_sensor.cpp)
ament_target_dependencies(pressureSensor rclcpp std_msgs)

add_executable(computer_vision src/computer_vision.cpp)
ament_target_dependencies(computer_vision rclcpp std_msgs sensor_msgs geometry_msgs cv_bridge)
target_link_libraries(computer_vision ${OpenCV_LIBS})

add_executable(image_capture src/image_capture.cpp)
ament_target_dependencies(image_capture rclcpp sensor_msgs cv_bridge)
target_link_libraries(image_capture ${OpenCV_LIBS})

add_executable(controller src/controller.cpp)
ament_target_dependencies(controller rclcpp std_msgs geometry_msgs)

add_executable(state_estimator src/state_estimator.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs)

# Install all executables
install(TARGETS
  pressureSensor
  computer_vision
  image_capture
  controller
  state_estimator
  planner
  snappy_cpp
  DESTINATION lib/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)


option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  # Add gtest executables
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(unit_tests tests/test_serial.cpp)
  ament_target_dependencies(unit_tests rclcpp)
  target_link_libraries(unit_tests serial_lib)
endif()

ament_package()
