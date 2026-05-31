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

    # Single shared TensorRT inference node serving both cameras (D455 front,
    # D405 bottom). Replaces the former front_cam + bottom_cam processes: one
    # CUDA context / engine for all cameras. Add a camera by appending its
    # namespace to camera_namespaces.
    camera_inference = TimerAction(
        period=3.0,
        actions=[
            Node(
                package="snappy_cpp",
                executable="camera_inference",
                name="camera_inference",
                output="screen",
                parameters=[
                    {"camera_namespaces": ["d455", "d405"]},
                    {"inference_hz": 10.0},
                    {"display": True},
                    {"distance_samples": 100},
                    # 0 => one queue slot per camera.
                    {"queue_depth": 0},
                    # Batch all cameras into one inference (0 => number of cameras).
                    # Only takes effect if the engine has a dynamic batch axis.
                    {"max_batch": 0},
                    # Coalescing window to let both cameras' frames join one batch.
                    # 0 => opportunistic (no added latency); try 5-15ms to batch more.
                    {"batch_collect_ms": 0.0},
                ],
            )
        ],
    )

    return LaunchDescription(
        [
            serial_dev_arg,
            micro_ros_agent,
            serial_no_d455_arg,
            serial_no_d405_arg,
            d455_launch,
            d405_launch,
            xsens_mti_node,
            camera_inference,
            pressure_sensor_node,
            planner_node,
            controller_node,
        ]
    )
