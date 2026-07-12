cmake_minimum_required(VERSION 3.5)
project(snappy_computer_vision)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(WITH_CUDA "Build with CUDA and TensorRT support" OFF)

include_directories(${Boost_INCLUDE_DIRS})

install(DIRECTORY config/
	DESTINATION share/${PROJECT_NAME}/config
)

install(DIRECTORY launch
	DESTINATION share/${PROJECT_NAME}
)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
# ThrusterCommand now lives in the shared snappy_interfaces package so the
# Jetson publisher and the STM32 micro-ROS subscriber register the SAME type.
find_package(snappy_interfaces REQUIRED)

# Generate message files
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BoundingBox2D.msg"
  "msg/ObjectDetection.msg"
  "msg/DetectionArray.msg"
  "msg/Polygon2D.msg"
  DEPENDENCIES std_msgs nav_msgs geometry_msgs builtin_interfaces
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# Find and require ONNX Runtime
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h PATHS /usr/include /usr/local/include REQUIRED)
find_library(ONNXRUNTIME_LIB onnxruntime PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu REQU
	IRED)

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

add_executable(image_capture src/image_capture.cpp)
ament_target_dependencies(image_capture rclcpp sensor_msgs cv_bridge)
target_link_libraries(image_capture ${OpenCV_LIBS})

if(WITH_CUDA)
  find_package(rosbridge_suite REQUIRED)
  find_package(CUDA REQUIRED)
  find_package(message_filters REQUIRED)
  # The CUDA nodes publish snappy_cpp::msg::DetectionArray (the type the
  # planner subscribes to), so they need snappy_cpp's generated headers.
  find_package(snappy_cpp REQUIRED)
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
  add_executable(front_camera_vision
    src/front_camera_vision.cpp
    src/preprocess_cuda.cu)
  target_include_directories(front_camera_vision PRIVATE
    src /usr/lib/aarch64-linux-gnu /usr/local/cuda/include)
  set_target_properties(front_camera_vision PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON)
  ament_target_dependencies(front_camera_vision rclcpp std_msgs sensor_msgs cv_bridge ament_index_cpp message_filters snappy_cpp)
  # Link to generated message library
  add_dependencies(front_camera_vision ${PROJECT_NAME})
  target_link_libraries(front_camera_vision
    ${PROJECT_NAME}__rosidl_typesupport_cpp
    ${PROJECT_NAME}__rosidl_typesupport_c
    ${NVINFER}
    ${NVONNXPARSER}
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
    /usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so
  )

  add_executable(bottom_camera_vision
    src/bottom_camera_vision.cpp
    src/preprocess_cuda.cu)
  target_include_directories(bottom_camera_vision PRIVATE
    src /usr/lib/aarch64-linux-gnu /usr/local/cuda/include)
  set_target_properties(bottom_camera_vision PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON)
  ament_target_dependencies(bottom_camera_vision rclcpp std_msgs sensor_msgs cv_bridge ament_index_cpp message_filters snappy_cpp)

  add_dependencies(bottom_camera_vision ${PROJECT_NAME})
  target_link_libraries(bottom_camera_vision
    ${PROJECT_NAME}__rosidl_typesupport_cpp
    ${PROJECT_NAME}__rosidl_typesupport_c
    ${NVINFER}
    ${NVONNXPARSER}
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
    /usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so
  )

  add_executable(displayFeed
    src/displayFeed.cpp)
  target_include_directories(displayFeed PRIVATE
    src /usr/lib/aarch64-linux-gnu /usr/local/cuda/include)
  set_target_properties(displayFeed PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON)
  ament_target_dependencies(displayFeed rclcpp sensor_msgs cv_bridge message_filters OpenCV snappy_cpp)
  add_dependencies(displayFeed ${PROJECT_NAME})
target_link_libraries(displayFeed
    ${PROJECT_NAME}__rosidl_typesupport_cpp
    ${PROJECT_NAME}__rosidl_typesupport_c
    ${NVINFER}
    ${NVONNXPARSER}
    ${CUDA_LIBRARIES}
    ${OpenCV_LIBS}
    /usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so
  )
endif()

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")


if(WITH_CUDA)
  install(TARGETS front_camera_vision
    DESTINATION lib/${PROJECT_NAME})
  install(TARGETS bottom_camera_vision
    DESTINATION lib/${PROJECT_NAME})
  install(TARGETS displayFeed
    DESTINATION lib/${PROJECT_NAME})
endif()

ament_package()
