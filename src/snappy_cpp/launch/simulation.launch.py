from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, SetEnvironmentVariable
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration

from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    package_share = get_package_share_directory('snappy_cpp')
    default_world = os.path.join(package_share, 'worlds', 'kraken.sdf')

    world = LaunchConfiguration('world')
    headless = LaunchConfiguration('headless')

    declare_world = DeclareLaunchArgument(
        'world',
        default_value=default_world,
        description='Absolute path to the Ignition Gazebo world (SDF) to load.',
    )

    declare_headless = DeclareLaunchArgument(
        'headless',
        default_value='false',
        description='Set to true to disable rendering (useful for CI).',
    )

    bridge_topics = [
        '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
        '/model/snappy/pose@geometry_msgs/msg/PoseStamped[gz.msgs.Pose',
    ]

    gz_sim_cmd = [
        'ign', 'gazebo',
        '-r',
        world,
    ]

    parameter_bridge_cmd = [
        'ros2', 'run', 'ros_gz_bridge', 'parameter_bridge',
    ] + bridge_topics

    set_headless_env = SetEnvironmentVariable(
        name='IGN_HEADLESS',
        value='1',
        condition=IfCondition(headless),
    )

    resource_paths = os.pathsep.join([
        package_share,
        os.path.join(package_share, 'worlds'),
    ])

    set_ign_resource_path = SetEnvironmentVariable(
        name='IGN_GAZEBO_RESOURCE_PATH',
        value=resource_paths,
    )

    set_gz_resource_path = SetEnvironmentVariable(
        name='GZ_SIM_RESOURCE_PATH',
        value=resource_paths,
    )

    gz_sim_process = ExecuteProcess(
        cmd=gz_sim_cmd,
        output='screen',
    )

    parameter_bridge_process = ExecuteProcess(
        cmd=parameter_bridge_cmd,
        output='screen',
    )

    return LaunchDescription([
        declare_world,
        declare_headless,
        set_ign_resource_path,
        set_gz_resource_path,
        set_headless_env,
        gz_sim_process,
        parameter_bridge_process,
    ])
