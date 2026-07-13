#!/bin/bash
set -e

echo "=== Snappy Nano Jetson Setup ==="

echo "=== Installing dependencies ==="
sudo apt-get update -q
sudo apt-get install -y \
    python3-vcstool \
    python3-colcon-common-extensions \
    python3-rosdep \
    nlohmann-json3-dev

echo "=== Importing repos ==="
cd ~/ros2_ws
vcs import src < snappy.repos

echo "=== Installing ROS dependencies ==="
rosdep update --rosdistro humble
rosdep install --from-paths src --ignore-src -y --skip-keys 'nlohmann_json'

echo "=== Building workspace ==="
source /opt/ros/humble/setup.bash
colcon build 

echo "=== Done! Run: source ~/.bashrc ==="
