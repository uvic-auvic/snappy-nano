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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include/Inc)

install(DIRECTORY config/
	DESTINATION share/${PROJECT_NAME}/config
)

install(DIRECTORY missions/
	DESTINATION share/${PROJECT_NAME}/missions
)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
# ThrusterCommand now lives in the shared snappy_interfaces package so the
# Jetson publisher and the STM32 micro-ROS subscriber register the SAME type.
find_package(snappy_interfaces REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)

# Generate message files
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Task.msg"
  "msg/ControllerStatus.msg"
  "msg/Pose.msg"
  "msg/BoundingBox2D.msg"
  "msg/ObjectDetection.msg"
  "msg/DetectionArray.msg"
  "msg/Polygon2D.msg"
  DEPENDENCIES std_msgs nav_msgs geometry_msgs builtin_interfaces
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

include_directories(
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

add_executable(state_estimator src/state_estimator.cpp src/include/src/kalman.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs nav_msgs tf2)
target_link_libraries(state_estimator Eigen3::Eigen)

# Offline KalmanFilter replay test (no ROS): replays recorded CSVs from a
# state_estimator_outputs folder through the filter and compares against the
# recorded kalman_*.csv. Run: test_kalman [path/to/state_estimator_outputs]
add_executable(test_kalman tests/test_kalman.cpp src/include/src/kalman.cpp)
target_link_libraries(test_kalman Eigen3::Eigen)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs yaml_cpp_vendor)
add_dependencies(planner ${PROJECT_NAME})

add_executable(sensorTest
    src/sensorTest.cpp)
ament_target_dependencies(sensorTest
  rclcpp
  std_msgs
  nav_msgs
  geometry_msgs
  Eigen3
  tf2
  tf2_geometry_msgs
  snappy_interfaces
)


add_executable(controller
  src/controller.cpp
  src/include/src/pid.cpp
  src/thruster_allocator.cpp
  src/include/src/motorboard.cpp
)
ament_target_dependencies(controller
  rclcpp
  std_msgs
  nav_msgs
  geometry_msgs
  Eigen3
  tf2
  tf2_geometry_msgs
  snappy_interfaces
)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(controller "${cpp_typesupport_target}")
target_link_libraries(sensorTest "${cpp_typesupport_target}")
target_link_libraries(state_estimator "${cpp_typesupport_target}")
target_link_libraries(planner "${cpp_typesupport_target}")

# Install all executables
install(TARGETS
  pressureSensor
  controller
  sensorTest
  state_estimator
  test_kalman
  planner
  DESTINATION lib/${PROJECT_NAME})

# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

install(DIRECTORY worlds
  DESTINATION share/${PROJECT_NAME})

ament_package()
