#!/bin/bash
# Jetson (ARM64) equivalent of setup-docker-snappy.sh.
# Run ON the Jetson (JetPack 6.x). Builds the local ARM64 image, starts the
# container with GPU + hardware access, and builds the ROS 2 workspace inside.
set -e

echo "=== Snappy Nano Jetson Docker Setup ==="

if [ "$(uname -m)" != "aarch64" ]; then
  echo "This script is for the Jetson (aarch64). On laptops use ./setup-docker-snappy.sh" >&2
  exit 1
fi

echo "=== Building Jetson image (ROS 2 Humble + CUDA 12.6/TensorRT base) ==="
docker compose --profile jetson build snappy-jetson

echo "=== Starting container ==="
docker compose --profile jetson up -d snappy-jetson

echo "=== Running setup inside container ==="
docker compose --profile jetson exec snappy-jetson bash -c "
  set -e &&
  cd /ros2_ws &&
  echo '--- Importing repos ---' &&
  vcs import src < snappy.repos &&
  vcs import src < src/waterlinked_dvl/ros2.repos &&
  echo '--- Installing dependencies ---' &&
  apt-get update -q &&
  rosdep update --rosdistro humble &&
  rosdep install --from-paths src --ignore-src -y --skip-keys 'nlohmann_json' &&
  echo '--- Building workspace ---' &&
  source /opt/ros/humble/setup.bash &&
  colcon build &&
  source install/local_setup.bash &&
  echo '=== Setup complete! ==='
"

echo "=== Dropping you into the container ==="
docker compose --profile jetson exec snappy-jetson bash
