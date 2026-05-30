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
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# Generate message files
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Task.msg"
  "msg/Pose.msg"
  "msg/BoundingBox2D.msg"
  "msg/ObjectDetection.msg"
  "msg/DetectionArray.msg"
  DEPENDENCIES std_msgs geometry_msgs builtin_interfaces
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# Find and require ONNX Runtime
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS /usr/include /usr/local/include REQUIRED)
find_library(ONNXRUNTIME_LIB onnxruntime PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu REQUIRED)

message(STATUS "Found ONNX Runtime: ${ONNXRUNTIME_LIB}")
message(STATUS "Found ONNX Runtime Headers: ${ONNXRUNTIME_INCLUDE_DIR}")
include_directories(
  ${ONNXRUNTIME_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)
link_directories(/usr/local/lib)

# Build all executables
set(BOOST_ROOT "/usr")
find_package(Boost REQUIRED)

add_library(serial_lib src/include/src/serial.cpp)
ament_target_dependencies(serial_lib rclcpp std_msgs)

add_executable(pressureSensor src/pressure_sensor.cpp)
ament_target_dependencies(pressureSensor rclcpp std_msgs)

add_executable(computer_vision_cpu src/computer_vision_cpu.cpp)
ament_target_dependencies(computer_vision_cpu rclcpp std_msgs sensor_msgs geometry_msgs cv_bridge ament_index_cpp)
target_link_libraries(computer_vision_cpu ${ONNXRUNTIME_LIB} ${OpenCV_LIBS})

add_executable(image_capture src/image_capture.cpp)
ament_target_dependencies(image_capture rclcpp sensor_msgs cv_bridge)
target_link_libraries(image_capture ${OpenCV_LIBS})

# add_executable(controller src/controller.cpp)
# ament_target_dependencies(controller rclcpp std_msgs geometry_msgs)

add_executable(state_estimator src/state_estimator.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs)
add_dependencies(planner ${PROJECT_NAME})
target_link_libraries(planner
  ${PROJECT_NAME}__rosidl_typesupport_cpp
  ${PROJECT_NAME}__rosidl_typesupport_c
)

add_executable(counter_publisher src/counter_publisher.cpp)
ament_target_dependencies(counter_publisher rclcpp std_msgs)

if(WITH_CUDA)
  find_package(rosbridge_suite REQUIRED)
  find_package(CUDA REQUIRED)
  find_package(message_filters REQUIRED)
  link_directories(/usr/local/cuda/targets/aarch64-linux/lib)

  # Compile the fused preprocessing kernel (.cu) with nvcc. Orin Nano is Ampere
  # (compute capability 8.7); override with -DCMAKE_CUDA_ARCHITECTURES if needed.
  # Set the architecture + policy BEFORE enable_language so it actually applies.
  if(POLICY CMP0104)
    cmake_policy(SET CMP0104 NEW)
  endif()
  if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 87)
  endif()
  enable_language(CUDA)

  find_library(NVINFER nvinfer PATHS /usr/lib/aarch64-linux-gnu)
  find_library(NVONNXPARSER nvonnxparser PATHS /usr/lib/aarch64-linux-gnu /usr/lib/x86_64-linux-gnu REQUIRED)

  # Single shared multi-camera inference node (replaces front_cam + bottom_cam,
  # which were byte-for-byte duplicate per-camera TensorRT pipelines).
  add_executable(camera_inference
    src/camera_inference.cpp
    src/preprocess_cuda.cu)
  target_include_directories(camera_inference PRIVATE
    src /usr/lib/aarch64-linux-gnu /usr/local/cuda/include)
  set_target_properties(camera_inference PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON)
  ament_target_dependencies(camera_inference rclcpp std_msgs sensor_msgs cv_bridge ament_index_cpp message_filters)

  # Link to generated message library
  add_dependencies(camera_inference ${PROJECT_NAME})
  target_link_libraries(camera_inference
    ${PROJECT_NAME}__rosidl_typesupport_cpp
    ${PROJECT_NAME}__rosidl_typesupport_c
    ${NVINFER}
    ${NVONNXPARSER}
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
    /usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so
  )
endif()

install(FILES
    src/yolo26n.onnx
    src/yolo26s.onnx
    DESTINATION share/${PROJECT_NAME}/models
)


add_executable(controller 
  src/controller.cpp
  src/include/src/pid.cpp
  src/include/src/motorboard.cpp
)
ament_target_dependencies(controller
  rclcpp
  std_msgs
  geometry_msgs
  tf2
  tf2_geometry_msgs
)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(controller "${cpp_typesupport_target}")
target_link_libraries(state_estimator "${cpp_typesupport_target}")
target_link_libraries(planner "${cpp_typesupport_target}")
#target_link_libraries(helloWorld "${cpp_typesupport_target}")
# install(TARGETS
#   helloWorld
#   controller
#   DESTINATION lib/${PROJECT_NAME})


# Install all executables
install(TARGETS
  pressureSensor
  computer_vision_cpu
  image_capture
  controller
  state_estimator
  planner
  counter_publisher
  DESTINATION lib/${PROJECT_NAME})

if(WITH_CUDA)
  install(TARGETS camera_inference
    DESTINATION lib/${PROJECT_NAME})
endif()

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

install(DIRECTORY worlds
  DESTINATION share/${PROJECT_NAME})

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(unit_tests tests/test_serial.cpp)
  ament_target_dependencies(unit_tests rclcpp)
  target_link_libraries(unit_tests serial_lib)
endif()

ament_package()
