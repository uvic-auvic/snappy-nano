cmake_minimum_required(VERSION 3.5)
project(snappy_cpp)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(WITH_CUDA "Build with CUDA and TensorRT support" OFF)
option(BUILD_TESTING "Build tests" OFF)

include_directories(${Boost_INCLUDE_DIRS})

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)

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

add_executable(state_estimator src/state_estimator.cpp src/kalman.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs)
target_link_libraries(state_estimator Eigen3::Eigen)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs)

add_executable(counter_publisher src/counter_publisher.cpp)
ament_target_dependencies(counter_publisher rclcpp std_msgs)

if(WITH_CUDA)
  find_package(CUDA REQUIRED)
  link_directories(/usr/local/cuda/targets/aarch64-linux/lib)

  find_library(NVINFER nvinfer PATHS /usr/lib/aarch64-linux-gnu)

  add_executable(cuda src/cuda.cpp)
  target_include_directories(cuda PRIVATE /usr/lib/aarch64-linux-gnu /usr/local/cuda/include)
  ament_target_dependencies(cuda rclcpp std_msgs)
  target_link_libraries(cuda
    ${NVINFER}
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
    /usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so
  )
endif()

# Install all executables
install(TARGETS
  pressureSensor
  computer_vision
  image_capture
  controller
  state_estimator
  planner
  counter_publisher
  snappy_cpp
  DESTINATION lib/${PROJECT_NAME})

if(WITH_CUDA)
  install(TARGETS cuda
    DESTINATION lib/${PROJECT_NAME})
endif()

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}/)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(unit_tests tests/test_serial.cpp)
  ament_target_dependencies(unit_tests rclcpp)
  target_link_libraries(unit_tests serial_lib)
endif()

ament_package()
