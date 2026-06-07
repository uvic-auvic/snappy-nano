#Set minimum required CMake version and project name.
cmake_minimum_required(VERSION 3.5)
project(xsens_mti_ros2_driver)


# Set C++ standard, default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(nmea_msgs REQUIRED)
find_package(mavros_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)

# NOTE: the bundled xspublic static libraries are built from source below
# (see "Build the bundled Xsens xspublic SDK"). We deliberately do NOT
# link_directories() against the committed prebuilt .a files because those
# are aarch64-only and break on x86_64.

# Check ROS distro
if(DEFINED ENV{ROS_DISTRO})
  set(ROS_DISTRO $ENV{ROS_DISTRO})
else()
  set(ROS_DISTRO "unknown")
endif()

message(STATUS "Detected ROS_DISTRO: ${ROS_DISTRO}")

################################################
## Declare ROS messages, services and actions ##
################################################


## Generate messages in the 'msg' folder

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/XsStatusWord.msg"
  "msg/ShipMotion.msg"
  DEPENDENCIES std_msgs
)


ament_export_dependencies(rosidl_default_runtime)

###########
## Build ##
###########


# === Build the bundled Xsens xspublic SDK from source ================
# Compile the xspublic sources directly with CMake instead of the old
# ExternalProject (which ran `make` against Makefiles that are never
# committed -- .gitignore ignores every file named "Makefile") or linking
# the committed prebuilt .a files (which are aarch64-only). This builds on
# any architecture: x86_64 dev box and aarch64 Jetson alike.
#
# Two xspublic quirks handled below:
#   * Its ".c" files must be compiled as C and ".cpp" as C++ (the default,
#     by extension). The SDK headers present each type as a plain struct to
#     C but as a class with "protected" members to C++; the C sources poke
#     those members directly, so compiling them as C++ would fail.
#   * Each library force-includes its config header (the SDK's MSVC /FI
#     option) which defines XS*_DLL_API used throughout the SDK headers.
set(XSPUBLIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/xspublic)

# Force-include the lib's config header (config_header) plus any extra headers
# passed as additional arguments, mirroring the SDK Makefiles' /FI flags. The
# xscommon sources additionally require xstypes/xsens_compat.h to be force-
# included: it maps Windows CRT names (e.g. _unlink -> unlink) and pulls in
# <string.h> (for strlen). The committed .dpp dependency files show the
# original build did this for xscommon only, so we replicate that exactly.
function(add_xspublic_lib name config_header)
  file(GLOB _srcs CONFIGURE_DEPENDS
    ${XSPUBLIC_DIR}/${name}/*.c
    ${XSPUBLIC_DIR}/${name}/*.cpp
  )
  add_library(${name} STATIC ${_srcs})
  target_include_directories(${name} PUBLIC
    ${XSPUBLIC_DIR}
    ${XSPUBLIC_DIR}/${name}
  )
  # Use "SHELL:" so CMake keeps each "-include <hdr>" pair intact; without it
  # CMake de-duplicates the repeated -include flag and the header gets treated
  # as a second input file ("cannot specify -o ... with multiple files").
  target_compile_options(${name} PRIVATE "SHELL:-include ${config_header}")
  foreach(_extra IN LISTS ARGN)
    target_compile_options(${name} PRIVATE "SHELL:-include ${_extra}")
  endforeach()
  set_target_properties(${name} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

add_xspublic_lib(xstypes      xstypesconfig.h)
add_xspublic_lib(xscommon     xscommon_config.h xstypes/xsens_compat.h)
add_xspublic_lib(xscontroller xscontrollerconfig.h)

# Inter-library dependencies: xscontroller -> xscommon -> xstypes.
target_link_libraries(xscommon     PUBLIC xstypes)
target_link_libraries(xscontroller PUBLIC xscommon xstypes)

## Specify additional locations of header files
## Your package locations should be listed before other locations
include_directories(
  ${PROJECT_SOURCE_DIR}/include/
  ${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/xspublic
  ${EIGEN3_INCLUDE_DIRS}
)


add_executable(
	xsens_mti_node
	src/main.cpp
	src/xdainterface.cpp
	src/xdacallback.cpp
	src/xsens_log_handler.cpp
	src/ntrip_util.cpp
	src/xsens_time_handler.cpp
  src/high_rate_interpolator.cpp
)

# Make sure xsens_mti_node depends on message generation (xspublic libs are
# linked as normal CMake targets below, so they don't need add_dependencies).
add_dependencies(xsens_mti_node ${PROJECT_NAME})


# target_include_directories(xsens_mti_node PUBLIC
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/xspublic>
#   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp> 
#   $<INSTALL_INTERFACE:include>)

ament_target_dependencies(xsens_mti_node 
	rclcpp
	tf2
	tf2_ros
	std_msgs
	geometry_msgs
	sensor_msgs
	nav_msgs
	nmea_msgs
	mavros_msgs
	rosidl_default_runtime
)

# Handle typesupport linking differently based on ROS distro
if(${ROS_DISTRO} STREQUAL "foxy")
  # For Foxy, directly link to the generated libraries
  target_link_libraries(xsens_mti_node
    ${CMAKE_BINARY_DIR}/libxsens_mti_ros2_driver__rosidl_typesupport_cpp.so
  )
  
  # Make sure the executable depends on message generation
  add_dependencies(xsens_mti_node 
    ${PROJECT_NAME}__rosidl_typesupport_cpp
  )
  
  # Add generated interfaces directory to include path
  target_include_directories(xsens_mti_node PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp
  )
else()
  # For Humble and later, use the built-in typesupport target function
  rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
  target_link_libraries(xsens_mti_node "${cpp_typesupport_target}")
endif()

target_link_libraries(
	xsens_mti_node
	xscontroller
	xscommon
	xstypes
	pthread
	dl
)



#############
## Install ##
#############

# all install targets should use catkin DESTINATION variables
# See http://ros.org/doc/api/catkin/html/adv_user_guide/variables.html

## Mark executables and/or libraries for installation
install(TARGETS xsens_mti_node
  EXPORT export_${PROJECT_NAME}
  DESTINATION lib/${PROJECT_NAME}
)


install(DIRECTORY launch param rviz urdf
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY headers/
  DESTINATION include/${PROJECT_NAME}
  FILES_MATCHING PATTERN "*.h"
)



#############
## Export  ##
#############
#This command is used to specify the dependencies that other packages need to know about 
#when they depend on this package. 
#This means if another package declares a build or run dependency on this package, 
#the dependencies listed inside ament_export_dependencies will be made available to that dependent package.
ament_export_dependencies(
	rclcpp
	tf2
	tf2_ros
	std_msgs
	geometry_msgs
	sensor_msgs
	nav_msgs
	nmea_msgs
	mavros_msgs
	rosidl_default_runtime #the default runtime for ROS 2 interface (message, service, action) generation.
  Eigen3
)

ament_export_include_directories(
	include
)


ament_package()
