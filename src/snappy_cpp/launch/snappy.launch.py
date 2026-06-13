from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

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
        # Sim-tuned PID gains, overridable per run without rebuilding
        # (e.g. pid_yaw_p:=20). The C++ defaults are NOT changed by these;
        # pool gains still live in controller.cpp.
        DeclareLaunchArgument(
            'pid_z_p',
            default_value='15.0',
            description='Depth PID proportional gain (thrust pct per metre).',
        ),
        DeclareLaunchArgument(
            'pid_yaw_p',
            default_value='15.0',
            description='Yaw PID proportional gain (thrust pct per radian).',
        ),
        DeclareLaunchArgument(
            'pid_yaw_d',
            default_value='0.1',
            description='Yaw PID derivative gain.',
        ),
        DeclareLaunchArgument(
            'pid_pitch_p',
            default_value='60.0',
            description='Pitch PID proportional gain (counters surge nose-down).',
        ),
        DeclareLaunchArgument(
            'pid_pitch_d',
            default_value='5.0',
            description='Pitch PID derivative gain.',
        ),
        DeclareLaunchArgument(
            'pid_roll_p',
            default_value='40.0',
            description='Roll PID proportional gain.',
        ),
        DeclareLaunchArgument(
            'pid_roll_d',
            default_value='2.0',
            description='Roll PID derivative gain.',
        ),
    ]

    ignition_simulation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(simulation_launch),
        launch_arguments={
            'world': world,
            'headless': headless,
        }.items(),
    )

    # The one controller binary, same as on the vehicle. sim_shim adapts it to
    # Gazebo: /motor_cmd -> gz cmd_thrust topics, gz altimeter/imu ->
    # depth_data + /filter/euler.
    controller_node = Node(
        package='snappy_cpp',
        executable='controller',
        name='controller',
        output='screen',
        condition=IfCondition(start_controller),
        parameters=[{
            'pid_z.p': ParameterValue(LaunchConfiguration('pid_z_p'), value_type=float),
            'pid_yaw.p': ParameterValue(LaunchConfiguration('pid_yaw_p'), value_type=float),
            'pid_yaw.d': ParameterValue(LaunchConfiguration('pid_yaw_d'), value_type=float),
            'pid_pitch.p': ParameterValue(LaunchConfiguration('pid_pitch_p'), value_type=float),
            'pid_pitch.d': ParameterValue(LaunchConfiguration('pid_pitch_d'), value_type=float),
            'pid_roll.p': ParameterValue(LaunchConfiguration('pid_roll_p'), value_type=float),
            'pid_roll.d': ParameterValue(LaunchConfiguration('pid_roll_d'), value_type=float),
        }],
    )

    sim_shim_node = Node(
        package='snappy_cpp',
        executable='sim_shim',
        name='sim_shim',
        output='screen',
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
            sim_shim_node,
            controller_node,
            state_estimator_node,
            planner_node,
            computer_vision_node,
            pressure_sensor_node,
        ]
    )
