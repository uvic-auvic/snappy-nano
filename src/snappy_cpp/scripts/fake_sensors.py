#!/usr/bin/env python3
"""Bench harness for the behaviour tree.

Closes the loop kinematically so the full mission tree can be walked with
thrusters unplugged: subscribes /planner/task and slews fake depth/heading
toward the commanded targets, publishes fake detections, and converges a
tracked object toward the image centre when the tree enables tracking.

    ros2 run snappy_cpp behavior_tree &        # or run in another terminal
    python3 fake_sensors.py
    ros2 topic echo /planner/task              # verify the Task stream

Checks to run by hand while it plays out:
  - every /planner/task message has overwrite=true
  - drive re-publish cadence <= 1 s while a Surge leg is RUNNING
  - Ctrl-C this script mid-run -> SensorsFreshOK trips -> EmergencySurface
  - after the root finishes, the tree latches (no mission restart)
"""
import math

import rclpy
from geometry_msgs.msg import Vector3Stamped
from rclpy.node import Node
from std_msgs.msg import Float32

from snappy_cpp.msg import BoundingBox2D, DetectionArray, ObjectDetection, Task

# What the fake cameras "see". Offsets are fractions of the frame from centre.
FRONT_OBJECTS = {
    "compass": 0.20,   # our-role gate icon (restore)
    "sos": 0.30,       # other-role icon, further right -> LatchGateSide says "left"
    "slalom": 0.15,
    "circle": 0.15,
}
DOWN_OBJECTS = {
    "nut_and_bolt": 0.18,  # role-matching bin icon (restore)
}
IMG_W, IMG_H = 848.0, 480.0

DEPTH_RATE = 0.3          # m/s slew toward depth target
TURN_RATE = math.radians(35.0)
TRACK_TAU = 1.2           # s, offset decay once tracking is enabled


def wrap(a):
    return math.atan2(math.sin(a), math.cos(a))


class FakeSensors(Node):
    def __init__(self):
        super().__init__("fake_sensors")
        self.depth = 0.0
        self.depth_target = 0.0
        self.heading = 0.0
        self.heading_target = 0.0
        self.tracked = None
        self.offsets = dict(FRONT_OBJECTS)
        self.offsets_down = dict(DOWN_OBJECTS)

        self.depth_pub = self.create_publisher(Float32, "depth_data", 10)
        self.euler_pub = self.create_publisher(Vector3Stamped, "/filter/euler", 10)
        self.front_pub = self.create_publisher(DetectionArray, "/d455/detections", 10)
        self.down_pub = self.create_publisher(DetectionArray, "/d405/detections", 10)
        self.create_subscription(Task, "/planner/task", self.on_task, 10)

        self.dt = 0.05
        self.create_timer(self.dt, self.step)
        self.get_logger().info("fake sensors up — depth/heading slew toward BT commands")

    def on_task(self, msg):
        self.get_logger().info(
            f"task: {msg.type}/{msg.direction} mag={msg.magnitude:.3f} "
            f"abs={msg.absolute} overwrite={msg.overwrite}")
        if not msg.overwrite:
            self.get_logger().error(
                "TASK WITHOUT overwrite=true — it would hit the controller's stalling queue!")
        if msg.type == "move" and msg.direction == "z":
            self.depth_target = msg.magnitude
        elif msg.type == "move" and msg.direction == "yaw":
            self.heading_target = wrap(msg.magnitude if msg.absolute
                                       else self.heading + msg.magnitude)
        elif msg.type == "track":
            self.tracked = msg.direction if msg.magnitude > 0 else None

    def step(self):
        self.depth += max(-DEPTH_RATE, min(DEPTH_RATE, self.depth_target - self.depth)) * self.dt
        err = wrap(self.heading_target - self.heading)
        self.heading = wrap(self.heading + max(-TURN_RATE, min(TURN_RATE, err / self.dt)) * self.dt)

        for table in (self.offsets, self.offsets_down):
            if self.tracked in table:
                table[self.tracked] *= math.exp(-self.dt / TRACK_TAU)

        self.depth_pub.publish(Float32(data=float(self.depth)))
        euler = Vector3Stamped()
        euler.header.stamp = self.get_clock().now().to_msg()
        euler.vector.z = math.degrees(self.heading)
        self.euler_pub.publish(euler)
        self.front_pub.publish(self.detections(self.offsets))
        self.down_pub.publish(self.detections(self.offsets_down))

    def detections(self, table):
        arr = DetectionArray()
        arr.header.stamp = self.get_clock().now().to_msg()
        for cls, off in table.items():
            det = ObjectDetection()
            det.object_class = cls
            det.confidence = 0.9
            det.distance_m = 3.0
            det.bounding_box = BoundingBox2D(
                x=float((0.5 + off) * IMG_W - 40), y=float(IMG_H / 2 - 40),
                width=80.0, height=80.0)
            arr.detections.append(det)
        return arr


def main():
    rclpy.init()
    node = FakeSensors()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    rclpy.shutdown()


if __name__ == "__main__":
    main()
