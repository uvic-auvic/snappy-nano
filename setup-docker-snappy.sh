#!/bin/bash
set -e

echo "=== Snappy Nano Setup ==="

# Automatically detect if the user is on a Mac or Linux/x86 laptop
if [[ "$(uname -s)" == "Darwin" ]]; then
  echo "macOS detected. Using native 'mac' profile."
  PROFILE="--profile mac"
  CONTAINER="snappy-mac"
  
  echo "=== Building and Starting container ==="
  docker compose $PROFILE up -d --build
else
  echo "🐧 Linux/x86 detected. Using default 'nano' profile."
  PROFILE=""
  CONTAINER="snappy-nano"
  
  echo "=== Pulling container ==="
  docker pull nil2thrill/snappy-ros:humble
  echo "=== Starting container ==="
  docker compose up -d
fi

echo "=== Cleaning stale build artifacts ==="
docker compose $PROFILE exec $CONTAINER bash -c "
  cd /ros2_ws &&
  rm -rf build/ install/ log/
"

echo "=== Running setup inside container ==="
docker compose $PROFILE exec $CONTAINER bash -c "
  set -e &&
  cd /ros2_ws &&
  export ROS_DISTRO=humble &&
  export ROS_PYTHON_VERSION=3 &&
  echo '--- Importing repos ---' &&
  vcs import src < snappy.repos &&
  vcs import src < src/waterlinked_dvl/ros2.repos &&
  echo '--- Installing dependencies ---' &&
  apt-get update -q &&
  apt-get install -y nlohmann-json3-dev &&
  rosdep update --rosdistro humble &&
  rosdep install --from-paths src --ignore-src -y --skip-keys 'nlohmann_json' --rosdistro humble &&
  echo '--- Building workspace ---' &&
  source /opt/ros/humble/setup.bash &&
  
  # Ensure interfaces build first, then build the rest sequentially to save memory
  colcon build --packages-select snappy_interfaces &&
  colcon build --executor sequential --cmake-args -DBUILD_TESTING=OFF &&
  
  source install/local_setup.bash &&
  echo '=== Setup complete! ==='
"

echo "=== Dropping you into the container ==="
docker compose $PROFILE exec $CONTAINER bash