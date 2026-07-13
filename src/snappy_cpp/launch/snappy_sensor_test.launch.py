"""
Launch File for snappy to test and make sure all our sensors have been booted up correctly so we can run the submarine
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
    EmitEvent,
    ExecuteProcess,
    IncludeLaunchDescription,
    OpaqueFunction,
    RegisterEventHandler,
    TimerAction,
)
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():

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

    parameters_file_path = Path(
        get_package_share_directory("xsens_mti_ros2_driver"),
        "param",
        "xsens_mti_node.yaml",
    )

    xsens_mti_node = Node(
        package="xsens_mti_ros2_driver",
        executable="xsens_mti_node",
        name="xsens_mti_node",
        output="screen",
        parameters=[parameters_file_path],
        arguments=[],
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

    # Bare Node reference needed for OnProcessExit to target
    sensor_test_node = Node(
        package="snappy_cpp",
        executable="sensorTest",
        name="sensorTest",
        output="screen",
    )

    # Delayed start to give sensors time to spin up
    sensor_test_timer = TimerAction(
        period=15.0,  # cameras + pressure sensor need time to initialize
        actions=[sensor_test_node],
    )

    def on_sensor_test_exit(event, context):
        # Only shut down the launch if sensorTest returned non-zero (failure)
        return [EmitEvent(event=Shutdown(reason="Sensors Did Not Work"))]

    sensor_test_exit_handler = RegisterEventHandler(
        event_handler=OnProcessExit(
            target_action=sensor_test_node,
            on_exit=on_sensor_test_exit,
        )
    )

    return LaunchDescription(
        [
            dvl,
            serial_dev_arg,
            micro_ros_agent,
            xsens_mti_node,
            pressure_sensor_node,
            sensor_test_timer,
            sensor_test_exit_handler,
        ]
    )
