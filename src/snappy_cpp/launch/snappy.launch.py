from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    package_share = get_package_share_directory('snappy_cpp')

    default_world = os.path.join(package_share, 'worlds', 'kraken.sdf')
    simulation_launch = os.path.join(package_share, 'launch', 'simulation.launch.py')

    world = LaunchConfiguration('world')
    headless = LaunchConfiguration('headless')
    start_controller = LaunchConfiguration('start_controller')
    start_state_estimator = LaunchConfiguration('start_state_estimator')
    start_computer_vision = LaunchConfiguration('start_computer_vision')
    start_pressure_sensor = LaunchConfiguration('start_pressure_sensor')
    start_planner = LaunchConfiguration('start_planner')

    arguments = [
        DeclareLaunchArgument(
            'world',
            default_value=default_world,
            description='SDF world to load in Ignition Gazebo.',
        ),
        DeclareLaunchArgument(
            'headless',
            default_value='false',
            description='Set true to disable rendering for CI/headless servers.',
        ),
        DeclareLaunchArgument(
            'start_controller',
            default_value='true',
            description='Launch the controller node.',
        ),
        DeclareLaunchArgument(
            'start_state_estimator',
            default_value='true',
            description='Launch the state estimator node.',
        ),
        DeclareLaunchArgument(
            'start_planner',
            default_value='false',
            description='Launch the planner node.',
        ),
        DeclareLaunchArgument(
            'start_computer_vision',
            default_value='false',
            description='Launch the computer vision node (requires GUI/OpenCV).',
        ),
        DeclareLaunchArgument(
            'start_pressure_sensor',
            default_value='false',
            description='Launch the pressure sensor serial node (requires hardware).',
        ),
    ]

    ignition_simulation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(simulation_launch),
        launch_arguments={
            'world': world,
            'headless': headless,
        }.items(),
    )

    controller_node = Node(
        package='snappy_cpp',
        executable='controller',
        name='controller',
        output='screen',
        condition=IfCondition(start_controller),
    )

    state_estimator_node = Node(
        package='snappy_cpp',
        executable='state_estimator',
        name='state_estimator',
        output='screen',
        condition=IfCondition(start_state_estimator),
    )

    planner_node = Node(
        package='snappy_cpp',
        executable='planner',
        name='planner',
        output='screen',
        condition=IfCondition(start_planner),
    )

    computer_vision_node = Node(
        package='snappy_cpp',
        executable='computer_vision',
        name='computer_vision',
        output='screen',
        condition=IfCondition(start_computer_vision),
    )

    pressure_sensor_node = Node(
        package='snappy_cpp',
        executable='pressureSensor',
        name='pressure_sensor',
        output='screen',
        condition=IfCondition(start_pressure_sensor),
    )

    return LaunchDescription(
        arguments
        + [
            ignition_simulation,
            controller_node,
            state_estimator_node,
            planner_node,
            computer_vision_node,
            pressure_sensor_node,
        ]
    )
