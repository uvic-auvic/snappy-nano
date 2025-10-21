"""
Launch file for Snappy ROV with RealSense camera.
Uses the official RealSense rs_launch.py and launches all Snappy C++ nodes.

Usage:
    ros2 launch snappy_cpp snappy_realsense.launch.py
"""

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    
    # Include the official RealSense launch file
    realsense_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([
                FindPackageShare('realsense2_camera'),
                'launch',
                'rs_launch.py'
            ])
        ]),
        launch_arguments={
            'camera_namespace': 'camera',
            'camera_name': 'camera',
            'enable_gyro': 'true',
            'enable_accel': 'true',
            'unite_imu_method': '2',  # 0=none, 1=copy, 2=linear_interpolation
            'enable_sync': 'true',
            'align_depth.enable': 'true',
        }.items()
    )
    
    # Delay starting the C++ nodes to let camera initialize (3 seconds)
    computer_vision_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='snappy_cpp',
                executable='computer_vision',
                name='computer_vision',
                output='screen',
            )
        ]
    )
    
    controller_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='snappy_cpp',
                executable='controller',
                name='controller',
                output='screen',
            )
        ]
    )
    
    state_estimator_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='snappy_cpp',
                executable='state_estimator',
                name='state_estimator',
                output='screen',
            )
        ]
    )
    
    planner_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='snappy_cpp',
                executable='planner',
                name='planner',
                output='screen',
            )
        ]
    )
    
    pressure_sensor_node = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='snappy_cpp',
                executable='pressureSensor',
                name='pressure_sensor',
                output='screen',
            )
        ]
    )

    return LaunchDescription([
        realsense_launch,
        computer_vision_node,
        controller_node,
        state_estimator_node,
        planner_node,
        pressure_sensor_node,
    ])
