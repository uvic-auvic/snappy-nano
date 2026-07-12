#!/bin/bash
# Sanity-check the snappy-jetson container environment. Run INSIDE the
# container:
#   docker compose --profile jetson exec snappy-jetson bash /ros2_ws/scripts/verify_jetson_container.sh
# Prints PASS/FAIL per check; exits non-zero if any check fails.

FAIL=0
check() {  # check <name> <command...>
  local name="$1"; shift
  if "$@" >/dev/null 2>&1; then
    echo "PASS  $name"
  else
    echo "FAIL  $name   (tried: $*)"
    FAIL=1
  fi
}

echo "== Architecture =="
[ "$(uname -m)" = "aarch64" ] && echo "PASS  arch is aarch64" || { echo "FAIL  arch is $(uname -m), expected aarch64"; FAIL=1; }

echo "== ROS 2 Humble =="
source /opt/ros/humble/setup.bash 2>/dev/null
check "ros2 CLI"             ros2 --help
check "ros2 daemon/topics"   timeout 20 ros2 topic list
check "demo_nodes_cpp"       ros2 pkg prefix demo_nodes_cpp

echo "== CUDA / GPU (Jetson iGPU has no nvidia-smi) =="
check "libcuda injected by nvidia runtime" ldconfig -p -N -X
python3 - <<'EOF' && echo "PASS  CUDA runtime sees a GPU" || { echo "FAIL  CUDA runtime sees no GPU"; FAIL=1; }
import ctypes, sys
cudart = ctypes.CDLL("libcudart.so")
n = ctypes.c_int(0)
rc = cudart.cudaGetDeviceCount(ctypes.byref(n))
sys.exit(0 if rc == 0 and n.value > 0 else 1)
EOF
check "nvcc"                 /usr/local/cuda/bin/nvcc --version
check "TensorRT libnvinfer"  sh -c 'ldconfig -p | grep -q libnvinfer'
check "trtexec"              /usr/src/tensorrt/bin/trtexec --help

echo "== GPIO =="
check "/dev/gpiochip0"       test -c /dev/gpiochip0
check "gpiodetect (libgpiod)" gpiodetect
check "gpioinfo chip 0"      gpioinfo gpiochip0
check "Jetson.GPIO importable" python3 -c "import Jetson.GPIO"
check "device-tree readable (Jetson.GPIO model detect)" cat /proc/device-tree/model

echo "== Buses & peripherals =="
check "I2C buses"            sh -c 'ls /dev/i2c-* && i2cdetect -l'
check "SPI"                  sh -c 'ls /dev/spidev*'
check "Jetson UART (ttyTHS)" sh -c 'ls /dev/ttyTHS*'
check "USB serial (ttyUSB/ttyACM)" sh -c 'ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep -q .'
check "USB enumeration"      lsusb
check "cameras (/dev/video*)" sh -c 'ls /dev/video*'
check "CAN interface (can0)" ip link show can0
check "PWM chips"            sh -c 'ls /sys/class/pwm/pwmchip*'

echo
if [ "$FAIL" -eq 0 ]; then
  echo "All checks passed."
else
  echo "Some checks FAILED (see above). USB-serial/camera/CAN failures may just mean that hardware is not plugged in."
fi
exit $FAIL
