# Snappy ROV - RealSense Launch Setup

## Overview
This project launches your complete Snappy ROV system with RealSense camera integration.

## What's Built
Your `snappy_cpp` package now includes these executables:
- **computer_vision** - Subscribes to RealSense camera/IMU topics, displays images
- **controller** - Controller logic (skeleton with basic logging)
- **state_estimator** - State estimation (skeleton with basic logging)
- **planner** - High-level planner/FSM (skeleton with basic logging)
- **pressureSensor** - Pressure sensor node
- **helloWorld** - Test node

## Launch File
`snappy_realsense.launch.py` launches:
1. RealSense camera node (realsense2_camera_node)
2. All your C++ nodes listed above

## Usage

### 1. Source your workspace
```bash
cd /home/kraken/snappy-nano/src
source install/setup.bash
```

### 2. Launch everything
```bash
ros2 launch snappy_cpp snappy_realsense.launch.py
```

This will:
- Start the RealSense camera
- Launch all your C++ nodes
- Display camera feeds in OpenCV windows (computer_vision node)
- Print status messages from each node

### 3. View topics (in another terminal)
```bash
# List all topics
ros2 topic list

# View camera image
ros2 run rqt_image_view rqt_image_view /camera/color/image_raw

# Echo IMU data
ros2 topic echo /camera/imu
```

## RealSense Topics Available
- `/camera/color/image_raw` - RGB camera stream
- `/camera/depth/image_rect_raw` - Depth stream
- `/camera/imu` - Combined IMU data
- `/camera/gyro/sample` - Gyroscope data
- `/camera/accel/sample` - Accelerometer data
- `/camera/aligned_depth_to_color/image_raw` - Aligned depth

## Node Status
- **computer_vision**: ✅ Subscribes to RealSense topics, displays images
- **controller**: ✅ Running with basic logging
- **state_estimator**: ✅ Running with basic logging
- **planner**: ✅ Running with basic logging
- **pressureSensor**: ✅ Running

## Next Steps
Now you can implement your actual logic in each C++ file:
1. Add control algorithms to `controller.cpp`
2. Add state estimation to `state_estimator.cpp`
3. Add mission planning to `planner.cpp`
4. Add vision processing/ML inference to `computer_vision.cpp`

All nodes are already integrated with ROS2 and ready to communicate!
