cmake_minimum_required(VERSION 3.5)
project(snappy_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Task.msg"
  "msg/Pose.msg"
  DEPENDENCIES "std_msgs" "geometry_msgs"
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

add_executable(helloWorld src/test.cpp)
ament_target_dependencies(helloWorld rclcpp std_msgs)

add_executable(controller 
  src/controller.cpp
  src/include/src/pid.cpp
  src/include/src/motorboard.cpp
)
ament_target_dependencies(controller
  rclcpp
  std_msgs
  geometry_msgs
  rosidl_default_generators
  tf2
  tf2_geometry_msgs
)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(controller "${cpp_typesupport_target}")

install(TARGETS
  helloWorld
  controller
  DESTINATION lib/${PROJECT_NAME})

ament_package()
