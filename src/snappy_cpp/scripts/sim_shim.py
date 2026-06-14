#!/usr/bin/env python3
"""Simulation shim: lets the real controller drive the Gazebo sub unchanged.

The controller node is sim-agnostic — it publishes ThrusterCommand on
/motor_cmd and reads depth_data + /filter/euler, exactly as on the vehicle.
This node stands in for the Mower Board firmware and the pressure/IMU
drivers:

  /motor_cmd (ThrusterCommand)  ->  8x /model/auv/joint/*/cmd_thrust (Float64, N)
  /altimeter (gz Altimeter)     ->  depth_data (Float32, metres, grows descending)
  /imu       (gz sensor_msgs)   ->  /filter/euler (Vector3Stamped, degrees)
"""

import math

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy

from geometry_msgs.msg import Vector3Stamped
from ros_gz_interfaces.msg import Altimeter
from sensor_msgs.msg import Imu
from snappy_interfaces.msg import ThrusterCommand
from std_msgs.msg import Float32, Float64


def gz_topic(joint):
    return f'/model/auv/joint/thruster_{joint}_joint/cmd_thrust'


# thrust_pct index -> (thruster joint, sign). Index i is firmware "Thruster
# i+1" (mask bit i); the layout mirrors Motorboard's speed arrays. Vertical
# thrusters are negated: the firmware convention is positive pct = descend,
# while the gz joints (axis 0 0 1) take positive thrust = up.
THRUSTER_MAP = {
    0: ('lateral_fore', 1.0),             # FRONT_YAW, + = +Y (port)
    1: ('vertical_starboard_fore', -1.0),  # FRONT_RIGHT
    2: ('forward_starboard', 1.0),         # FORWARD_RIGHT, + = surge fwd
    3: ('vertical_starboard_aft', -1.0),   # BACK_RIGHT
    4: ('lateral_aft', 1.0),               # BACK_YAW, + = -Y (starboard)
    5: ('vertical_port_aft', -1.0),        # BACK_LEFT
    6: ('forward_port', 1.0),              # FORWARD_LEFT
    7: ('vertical_port_fore', -1.0),       # FRONT_LEFT
}


class SimShim(Node):
    def __init__(self):
        super().__init__('sim_shim')

        # T200 peak thrust is ~50 N, so 100 pct ~= 40 N is a conservative map.
        self.newtons_per_pct = self.declare_parameter(
            'newtons_per_pct', 0.4).value
        # Depth of the spawn pose below the surface: kraken.sdf spawns the auv
        # at z=-2 and the gz altimeter reports displacement from spawn, so
        # depth = initial_depth - vertical_position.
        self.initial_depth = self.declare_parameter('initial_depth', 2.0).value

        self.thruster_pubs = {
            i: self.create_publisher(Float64, gz_topic(joint), 10)
            for i, (joint, _) in THRUSTER_MAP.items()
        }
        self.depth_pub = self.create_publisher(Float32, 'depth_data', 10)
        self.euler_pub = self.create_publisher(Vector3Stamped, '/filter/euler', 10)

        # The controller publishes /motor_cmd best-effort (to match the STM32
        # micro-ROS subscriber), so this subscription must be best-effort too.
        self.create_subscription(
            ThrusterCommand, '/motor_cmd', self.motor_cmd_callback,
            QoSProfile(depth=10, reliability=ReliabilityPolicy.BEST_EFFORT))
        self.create_subscription(
            Altimeter, '/altimeter', self.altimeter_callback, 10)
        self.create_subscription(Imu, '/imu', self.imu_callback, 10)

    def motor_cmd_callback(self, msg):
        for i, (_, sign) in THRUSTER_MAP.items():
            if msg.thruster_mask & (1 << i):
                self.thruster_pubs[i].publish(Float64(
                    data=sign * self.newtons_per_pct * float(msg.thrust_pct[i])))

    def altimeter_callback(self, msg):
        self.depth_pub.publish(Float32(
            data=self.initial_depth - msg.vertical_position))

    def imu_callback(self, msg):
        q = msg.orientation
        # ZYX euler, the same convention the Xsens /filter/euler topic uses.
        roll = math.atan2(2.0 * (q.w * q.x + q.y * q.z),
                          1.0 - 2.0 * (q.x * q.x + q.y * q.y))
        pitch = math.asin(max(-1.0, min(1.0, 2.0 * (q.w * q.y - q.z * q.x))))
        yaw = math.atan2(2.0 * (q.w * q.z + q.x * q.y),
                         1.0 - 2.0 * (q.y * q.y + q.z * q.z))

        euler = Vector3Stamped()
        euler.header = msg.header
        euler.vector.x = math.degrees(roll)
        euler.vector.y = math.degrees(pitch)
        euler.vector.z = math.degrees(yaw)
        self.euler_pub.publish(euler)


def main():
    rclpy.init()
    rclpy.spin(SimShim())


if __name__ == '__main__':
    main()
