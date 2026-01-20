# This is the Readme file for the snappy nano project

- this is a c++ ros2 project that is going to be used to run AUVIC's submarine for this years competition cycle

## Build project
The workspace assumes ROS 2 Humble is already installed.

```bash
cd ~/auvic/snappy-nano
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install --packages-select snappy_cpp
source install/setup.bash
```

## Run Ignition Gazebo by itself

```bash
ros2 launch snappy_cpp simulation.launch.py headless:=false world:=<path/to/world.sdf>
```

`simulation.launch.py` loads `worlds/kraken.sdf`, exports the world + mesh paths, and starts the `ros_gz_bridge` parameters listed in the file so ROS topics such as `/thrusters/<name>/cmd` are available immediately.

## Run the simulator with the ROS stack

```bash
ros2 launch snappy_cpp snappy.launch.py \
  start_planner:=false \
  start_computer_vision:=false \
  start_pressure_sensor:=false
```

Arguments let teammates choose which nodes to launch:

| Argument | Default | Purpose |
| --- | --- | --- |
| `world` | `worlds/kraken.sdf` | Path to the Ignition Gazebo world. |
| `headless` | `false` | Set `true` to disable the Gazebo GUI. |
| `start_controller` | `true` | Run `snappy_cpp/controller`. |
| `start_state_estimator` | `true` | Run `snappy_cpp/state_estimator`. |
| `start_planner` | `false` | Run the planner stub. |
| `start_computer_vision` | `false` | Enable OpenCV/RealSense processing (requires a GUI). |
| `start_pressure_sensor` | `false` | Enable the serial pressure sensor node (needs hardware). |

The launch file automatically includes `simulation.launch.py`, so one command brings up Ignition Gazebo, the ROS⇄Gazebo bridge, and the requested ROS 2 nodes for testing against the virtual vehicle.
