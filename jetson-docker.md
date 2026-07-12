# Running snappy-nano in Docker on the Jetson Orin Nano

The Docker Hub image used by laptops (`nil2thrill/snappy-ros:humble`) is
**amd64-only**, so it cannot run on the Jetson. The Jetson gets its own image,
built locally from `Dockerfile.jetson` on top of NVIDIA's JetPack container
(`nvcr.io/nvidia/l4t-jetpack:r36.4.0`): Ubuntu 22.04 + ROS 2 Humble + CUDA 12.6
+ TensorRT + cuDNN — matching the CUDA 12.6 / compute 8.7 (Orin) settings in
`src/snappy_computer_vision/CMakeLists.txt`.

The laptop workflow is untouched: `docker compose up -d` / `./setup-docker-snappy.sh`
still use the `snappy-nano` service and the Hub image. The Jetson service is
behind a compose **profile** so it never starts by accident on laptops.

## Host requirements (already true on our Orin Nano)

- JetPack 6.x (L4T r36.4), Ubuntu 22.04
- Docker with the NVIDIA Container Toolkit (`sudo apt install nvidia-container-toolkit`,
  then `sudo nvidia-ctk runtime configure --runtime=docker && sudo systemctl restart docker`)
- Your user in the `docker` group

## Build on the Jetson

```bash
cd ~/snappy-nano
./setup-docker-snappy-jetson.sh     # build image + start container + colcon build, one shot
```

or step by step:

```bash
docker compose --profile jetson build snappy-jetson
docker compose --profile jetson up -d snappy-jetson
docker compose --profile jetson exec snappy-jetson bash
```

## What the container gets

| Capability | How |
| --- | --- |
| GPU / CUDA / TensorRT / EGL / multimedia | `runtime: nvidia` + `NVIDIA_DRIVER_CAPABILITIES=all` inject the Tegra driver stack; CUDA 12.6 + TensorRT come from the JetPack base image |
| GPIO | `/dev/gpiochip0` (main) and `/dev/gpiochip1` (AON) via the `/dev` bind + `privileged`. Tools: `libgpiod` (C++ dev headers included), `gpiod` CLI, Python `Jetson.GPIO` and `python3-libgpiod`. sysfs GPIO is deprecated on JetPack 6 — use the character device. Do **not** use `RPi.GPIO`. |
| I2C / SPI | `/dev/i2c-*`, `/dev/spidev*` via `/dev` bind; `i2c-tools`, `python3-smbus` installed |
| UART / serial | `/dev/ttyTHS1` (40-pin header UART), `/dev/ttyUSB*`, `/dev/ttyACM*` via `/dev` bind (hot-plug works because the whole `/dev` is bound, not individual nodes) |
| CAN | `network_mode: host` exposes SocketCAN `can0`; `can-utils` installed. Bring the interface up on the **host**: `sudo ip link set can0 up type can bitrate 500000` |
| PWM | `/sys/class/pwm/pwmchip*` writable because of `privileged` |
| USB devices / cameras | `/dev` bind (`/dev/video*`, `lsusb`); RealSense works over USB, CSI cameras via the `/tmp/argus_socket` mount |
| ROS 2 DDS | host networking, same as the laptop setup, so discovery works with nodes on the host and other machines on the LAN |
| GUI (rqt/RViz) | X11 socket mount + `DISPLAY`; run `xhost +local:docker` on the host first (only when the Jetson runs a desktop) |

## Verify

Inside the container (`docker compose --profile jetson exec snappy-jetson bash`):

```bash
# everything at once:
bash /ros2_ws/scripts/verify_jetson_container.sh

# or manually:
uname -m                                  # aarch64
ros2 topic list
ros2 run demo_nodes_cpp talker            # terminal 1
ros2 run demo_nodes_cpp listener          # terminal 2 (or on the host — DDS over host net)

# GPIO (Python)
python3 -c "import Jetson.GPIO as GPIO; GPIO.setmode(GPIO.BOARD); print(GPIO.model)"
# GPIO (CLI / what C++ libgpiod sees)
gpiodetect            # should list tegra234-gpio + tegra234-gpio-aon
gpioinfo gpiochip0

# CUDA (Jetson iGPU has no nvidia-smi — use these instead)
python3 -c "import ctypes;c=ctypes.CDLL('libcudart.so');n=ctypes.c_int();c.cudaGetDeviceCount(ctypes.byref(n));print('CUDA devices:',n.value)"
/usr/local/cuda/bin/nvcc --version
/usr/src/tensorrt/bin/trtexec --help | head -1
# on the HOST, GPU load/health: sudo pip3 install jetson-stats && jtop
```

## Build the vision stack with CUDA

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select snappy_interfaces snappy_computer_vision \
  --cmake-args -DWITH_CUDA=ON
```

## VS Code

"Reopen in Container" now offers two configs: the default x86 one and
**snappy-nano Jetson (ROS 2 Humble, ARM64)** (`.devcontainer/jetson/`). Pick the
Jetson one when VS Code is attached to the Jetson (e.g. via Remote-SSH).

## Surviving reboots / running the same every time

- The service has `restart: unless-stopped`, and `docker.service` is enabled at
  boot — so after a reboot the **same container** comes back up by itself.
  `docker compose --profile jetson exec snappy-jetson bash` and you're back in.
- The workspace (`src/`, `build/`, `install/`, `log/`) is a bind mount of this
  repo on the host — it survives reboots *and* container recreation. Nothing
  built with colcon is ever lost.
- What does NOT survive is anything `apt-get install`ed by hand inside the
  container, and only if the container is **recreated** (`docker compose down`,
  or `up -d` after editing compose/Dockerfile) — a plain reboot or
  `stop`/`start` keeps it. After a recreate, just re-run
  `./setup-docker-snappy-jetson.sh`: it reinstalls the rosdep set and rebuilds,
  landing you in an identical state. Anything you find yourself installing by
  hand repeatedly belongs in `Dockerfile.jetson` instead.
- SocketCAN is host state: `can0` comes up unconfigured on every boot. If you
  use CAN, add a systemd oneshot on the host (`ip link set can0 up type can
  bitrate 500000`) — the container just sees whatever the host configured.

## Gotchas

- The first build downloads the ~10 GB JetPack base image; give it time.
- `nvidia-smi` does not exist on Jetson (integrated GPU) — that's expected.
- If DDS discovery between container and host fails, both sides must use the
  same `ROS_DOMAIN_ID` (unset = 0 on both by default).
- Keep the base image L4T major version (r36.x) matched to the host JetPack 6;
  don't bump it to an r38 tag when JetPack 7 hosts appear without testing.
