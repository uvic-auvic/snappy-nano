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
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # Launch arguments for serial numbers (with defaults)
    # Serial numbers must be wrapped in single quotes for RealSense parameter type handling
    serial_no_d455_arg = DeclareLaunchArgument(
        "serial_no_d455",
        default_value="'239222301226'",
        description="Serial number of D455 camera",
    )

    serial_no_d405_arg = DeclareLaunchArgument(
        "serial_no_d405",
        default_value="'230422271334'",
        description="Serial number of D405 camera",
    )

    # Get launch configuration
    serial_no_d455 = LaunchConfiguration("serial_no_d455")
    serial_no_d405 = LaunchConfiguration("serial_no_d405")

    # D455 Front Camera
    d455_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                PathJoinSubstitution(
                    [FindPackageShare("realsense2_camera"), "launch", "rs_launch.py"]
                )
            ]
        ),
        launch_arguments={
            "camera_namespace": "d455",
            "camera_name": "d455",
            "serial_no": serial_no_d455,
            "enable_gyro": "true",
            "enable_accel": "true",
            "unite_imu_method": "2",
            "enable_sync": "true",
            "align_depth.enable": "true",
        }.items(),
    )

    # D405 Bottom Camera
    d405_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            [
                PathJoinSubstitution(
                    [FindPackageShare("realsense2_camera"), "launch", "rs_launch.py"]
                )
            ]
        ),
        launch_arguments={
            "camera_namespace": "d405",
            "camera_name": "d405",
            "serial_no": serial_no_d405,
            "enable_gyro": "false",  # D405 does not include IMU
            "enable_accel": "false",
            "unite_imu_method": "0",
            "enable_sync": "true",
            "align_depth.enable": "true",
        }.items(),
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

    # Delay starting the C++ nodes to let camera initialize (3 seconds)
    computer_vision_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="computer_vision",
                name="computer_vision",
                output="screen",
            )
        ],
    )

    controller_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="controller",
                name="controller",
                output="screen",
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

    image_capture_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="image_capture",
                name="image_capture",
                output="screen",
            )
        ],
    )

    # Front camera node (D455) with proper namespace
    front_cam = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="front_cam",
                name="front_cam",
                output="screen",
                parameters=[
                    {"camera_namespace": "d455"},
                ],
            )
        ],
    )

    # Bottom camera node (D405) with proper namespace
    bottom_cam = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="bottom_cam",
                name="bottom_cam",
                output="screen",
                parameters=[
                    {"camera_namespace": "d405"},
                ],
            )
        ],
    )

    return LaunchDescription(
        [
            serial_no_d455_arg,
            serial_no_d405_arg,
            d455_launch,
            d405_launch,
            xsens_mti_node,
            front_cam,
            bottom_cam,
            planner_node,
        ]
    )
