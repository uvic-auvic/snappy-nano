docker run -it --rm \
  --runtime nvidia \
  --network host \
  -v ~/workspaces/isaac_ros-dev:/workspaces/isaac_ros-dev \
  isaac_ros:humble-orin

