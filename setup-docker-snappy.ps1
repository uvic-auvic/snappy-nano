$ErrorActionPreference = "Stop"

Write-Host "=== Snappy Nano Setup ==="

Write-Host "=== Pulling container ==="
docker pull nil2thrill/snappy-ros:humble

Write-Host "=== Starting container ==="
docker compose up -d

Write-Host "=== Running setup inside container ==="
docker compose exec snappy-nano bash -c "
  set -e &&
  cd /ros2_ws &&
  echo '--- Importing repos ---' &&
  vcs import src < snappy.repos &&
  vcs import src < src/waterlinked_dvl/ros2.repos &&
  echo '--- Installing dependencies ---' &&
  apt-get update -q &&
  apt-get install -y nlohmann-json3-dev &&
  rosdep update --rosdistro humble &&
  rosdep install --from-paths src --ignore-src -y --skip-keys 'nlohmann_json' &&
  echo '--- Building workspace ---' &&
  source /opt/ros/humble/setup.bash &&
  colcon build &&
  source install/local_setup.bash &&
  echo '=== Setup complete! ==='
"

if ($LASTEXITCODE -ne 0) {
    Write-Error "Setup failed inside container (exit code $LASTEXITCODE)"
    exit $LASTEXITCODE
}

Write-Host "=== Dropping you into the container ==="
docker compose exec snappy-nano bash
