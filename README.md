# This is the Readme file for the snappy nano project

- this is a c++ ros2 project that is going to be used to run AUVIC's submarine for this years competition cycle

### Subsystem: [Planner, State Estimator, Controller, Simulation]

### Overview

This branch combines the updated state estimator, PID controller node, and gazebo to allow work with data in the gazebo simulator.

This build assumes Gazebo Ignition Fortress is installed on version 6.17.1

You must also have the ros_gz_interfaces package installed for the sim controller to use the altimeter for depth.

### Details

##Build Project
```bash
cd ~/snappy-nano
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install --packages-select snappy_cpp
source install/setup.bash
```
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



For manual control of the motors while the sim is running all motors follow a similar convention to:
```bash
/model/auv/joint/thruster_forward_port_joint/cmd_thrust
```
all topics take Float64 as a message representing the rad/s which gets converted to Newtons in the sim.