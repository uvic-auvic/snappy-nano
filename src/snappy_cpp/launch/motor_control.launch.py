"""Launch the micro-ROS agent and MotorCommandPublisher together.

Usage:
    ros2 launch snappy_cpp motor_control.launch.py

Optionally override serial port or motor parameters:
    ros2 launch snappy_cpp motor_control.launch.py serial_dev:=/dev/ttyTHS1
"""
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    serial_dev_arg = DeclareLaunchArgument(
        'serial_dev', default_value='/dev/ttyUSB0',
        description='Serial device for micro-ROS agent')

    micro_ros_agent = ExecuteProcess(
        cmd=[
            'ros2', 'run', 'micro_ros_agent', 'micro_ros_agent',
            'serial',
            '--dev', LaunchConfiguration('serial_dev'),
            '-b', '115200',
        ],
        output='screen',
    )

    motor_cmd_publisher = Node(
        package='snappy_cpp',
        executable='controller',
        name='motor_command_publisher',
        output='screen',
        parameters=[{
            'motor_select': 255.0,
            'speed': 0.0,
        }],
    )

    return LaunchDescription([
        serial_dev_arg,          # Specify the serial device
        micro_ros_agent,         # Start the micro-ROS agent
        motor_cmd_publisher,     # Start the controller node
    ])
