"""
Launch file for Snappy ROV with RealSense cameras.
Uses the official RealSense rs_launch.py and launches all Snappy C++ nodes.

Usage:
    ros2 launch snappy_cpp snappy_realsense.launch.py
    ros2 launch snappy_cpp snappy_realsense.launch.py serial_no_d455:='<serial>' serial_no_d405:='<serial>'
"""

from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    ExecuteProcess,
    IncludeLaunchDescription,
    TimerAction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare



def generate_launch_description():

    snappyComputerVision = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                PathJoinSubstitution(
                    [
                        FindPackageShare("snappy_computer_vision"),
                        "launch",
                        "snappy_computer_vision.launch.py",
                    ]
                )
            ]
        ),
    )

    dvl = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                PathJoinSubstitution(
                    [
                        FindPackageShare("waterlinked_dvl_driver"),
                        "launch",
                        "dvl.launch.py",
                    ]
                )
            ]
        ),
    )

    serial_dev_arg = DeclareLaunchArgument(
        "serial_dev",
        default_value="/dev/ttyUSB1",
        description="Serial device for micro-ROS agent",
    )

    micro_ros_agent = ExecuteProcess(
        cmd=[
            "ros2",
            "run",
            "micro_ros_agent",
            "micro_ros_agent",
            "serial",
            "--dev",
            LaunchConfiguration("serial_dev"),
            "-b",
            "115200",
        ],
        output="screen",
    )

    xsens_parameters_file_path = Path(
        get_package_share_directory("xsens_mti_ros2_driver"),
        "param",
        "xsens_mti_node.yaml",
    )
    controller_parameters_file_path = Path(
        get_package_share_directory("snappy_cpp"),
        "config",
        "controller_params.yaml",
    )

    state_estimator_parameters_file_path = Path(
        get_package_share_directory("snappy_cpp"),
        "config",
        "state_estimator_params.yaml",
    )

    xsens_mti_node = Node(
        package="xsens_mti_ros2_driver",
        executable="xsens_mti_node",
        name="xsens_mti_node",
        output="screen",
        parameters=[xsens_parameters_file_path],
        arguments=[],
    )

    controller_node = TimerAction(
        period=8.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="controller",
                name="controller",
                output="screen",
                parameters=[controller_parameters_file_path]
            )
        ],
    )

    state_estimator_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="state_estimator",
                name="state_estimator",
                output="screen",
                parameters=[state_estimator_parameters_file_path]
            )
        ],
    )

    planner_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="planner",
                name="planner",
                output="screen",
            )
        ],
    )

    pressure_sensor_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="pressureSensor",
                name="pressure_sensor",
                output="screen",
            )
        ],
    )

    return LaunchDescription(
        [
            xsens_mti_node,
            #snappyComputerVision,
            dvl,
            serial_dev_arg,
            micro_ros_agent,
            pressure_sensor_node,
            planner_node,
            state_estimator_node,
            controller_node,
        ]
    )
