cmake_minimum_required(VERSION 3.5)
project(snappy_cpp)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(${Boost_INCLUDE_DIRS})

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
set(BOOST_ROOT "/usr")
find_package(Boost REQUIRED)

add_executable(snappy_cpp src/test.cpp)
ament_target_dependencies(snappy_cpp rclcpp std_msgs)
target_link_libraries(snappy_cpp ${Boost_LIBRARIES})

install(TARGETS
  snappy_cpp
  DESTINATION lib/${PROJECT_NAME})

option(BUILD_TESTING "Build tests" OFF)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  # Add gtest executables
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(unit_tests tests/test_motorboard.cpp)
  ament_target_dependencies(unit_tests rclcpp)
  #target_link_libraries(unit_tests <library-name>)

endif()


ament_package()