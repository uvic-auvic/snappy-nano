#!/bin/bash
set -e

echo "=== Snappy Nano Setup ==="

# Build and start the container

echo "=== Get Container & Build ==="
docker pull nil2thrill/snappy-ros:humble
docker compose up -d

# Import all repos into src/
echo "=== Get Container & Build ==="
docker compose exec snappy-nano bash -c "
  cd /ros2_ws &&
  vcs import < snappy.repos &&
  rosdep update &&
  rosdep install --from-paths src --ignore-src -y
"

echo "=== Build The Project ==="
# Build the workspace
docker compose exec snappy-nano bash -c "
  source /opt/ros/humble/setup.bash &&
  cd /ros2_ws &&
  colcon build --packages-ignore micro_ros_setup &&
  source install/local_setup.bash &&
  colcon build --packages-select micro_ros_setup &&
  source install/local_setup.bash &&
  ros2 run micro_ros_setup create_agent_ws.sh &&
  ros2 run micro_ros_setup build_agent.sh
"

echo "=== Setup complete! ==="
echo "Run: docker compose exec snappy-nano bash"
