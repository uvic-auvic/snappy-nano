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
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Task.msg"
  "msg/Pose.msg"
  DEPENDENCIES "std_msgs" "geometry_msgs"
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# Build all executables
set(BOOST_ROOT "/usr")
find_package(Boost REQUIRED)

add_library(serial_lib src/include/src/serial.cpp)
ament_target_dependencies(serial_lib rclcpp std_msgs)

add_executable(helloWorld src/test.cpp)
set_target_properties(helloWorld PROPERTIES OUTPUT_NAME "snappy_cpp")
ament_target_dependencies(helloWorld rclcpp std_msgs)
target_link_libraries(helloWorld ${Boost_LIBRARIES})

add_executable(pressureSensor src/pressure_sensor.cpp)
ament_target_dependencies(pressureSensor rclcpp std_msgs)

add_executable(computer_vision src/computer_vision.cpp)
ament_target_dependencies(computer_vision rclcpp std_msgs sensor_msgs geometry_msgs cv_bridge)
target_link_libraries(computer_vision ${OpenCV_LIBS})

# add_executable(controller src/controller.cpp)
# ament_target_dependencies(controller rclcpp std_msgs geometry_msgs)

add_executable(state_estimator src/state_estimator.cpp)
ament_target_dependencies(state_estimator rclcpp std_msgs geometry_msgs sensor_msgs)

add_executable(planner src/planner.cpp)
ament_target_dependencies(planner rclcpp std_msgs geometry_msgs)


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
target_link_libraries(planner "${cpp_typesupport_target}")
target_link_libraries(helloWorld "${cpp_typesupport_target}")
# install(TARGETS
#   helloWorld
#   controller
#   DESTINATION lib/${PROJECT_NAME})


# Install all executables
install(TARGETS
  pressureSensor
  computer_vision
  controller
  state_estimator
  planner
  helloWorld
  DESTINATION lib/${PROJECT_NAME}
)
# Install launch files
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

install(DIRECTORY worlds
  DESTINATION share/${PROJECT_NAME})

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
