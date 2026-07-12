# snappy-nano image for NVIDIA Jetson (ARM64) — JetPack 6.x / L4T r36.4 hosts.
#
# The x86 laptop workflow keeps using ./Dockerfile (nil2thrill/snappy-ros:humble,
# which is amd64-only). This file is the ARM64 equivalent, based on NVIDIA's
# JetPack container: Ubuntu 22.04 + CUDA 12.6 + TensorRT + cuDNN + VPI. That
# matches the /usr/local/cuda-12.6 paths and CUDA arch 8.7 already hardcoded in
# src/snappy_computer_vision/CMakeLists.txt. The Tegra driver stack (libcuda,
# nvgpu, DLA, multimedia) is injected at run time by the NVIDIA container
# runtime — do NOT bake it into the image.
FROM nvcr.io/nvidia/l4t-jetpack:r36.4.0

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8

# ROS 2 Humble from the OSRF apt repo (Ubuntu 22.04 "jammy" ships arm64 debs),
# plus the build tooling the setup scripts expect.
RUN apt-get update && apt-get install -y --no-install-recommends \
        curl gnupg lsb-release locales software-properties-common && \
    locale-gen en_US en_US.UTF-8 && update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 && \
    add-apt-repository universe && \
    curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
        -o /usr/share/keyrings/ros-archive-keyring.gpg && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" \
        > /etc/apt/sources.list.d/ros2.list && \
    # Kitware repo: jammy's stock cmake is 3.22, but libwaterlinked (pulled in
    # by waterlinked_dvl/ros2.repos) requires cmake >= 3.24.
    curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc \
        | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $(. /etc/os-release && echo $UBUNTU_CODENAME) main" \
        > /etc/apt/sources.list.d/kitware.list && \
    apt-get update && apt-get install -y --no-install-recommends \
        ros-humble-ros-base \
        ros-humble-demo-nodes-cpp \
        ros-dev-tools \
        python3-rosdep \
        python3-vcstool \
        python3-colcon-common-extensions \
        python3-pip \
        build-essential \
        cmake \
        git \
        nlohmann-json3-dev && \
    rm -rf /var/lib/apt/lists/*

# Hardware access tooling. Jetson GPIO on JetPack 6 is the character device
# (/dev/gpiochip0 = main "tegra234-gpio", /dev/gpiochip1 = AON); the sysfs GPIO
# interface is deprecated. libgpiod covers C++, Jetson.GPIO covers Python with
# the familiar RPi.GPIO-style API (it is NOT RPi.GPIO — pin muxing differs).
RUN apt-get update && apt-get install -y --no-install-recommends \
        gpiod libgpiod-dev python3-libgpiod \
        i2c-tools python3-smbus \
        can-utils iproute2 \
        python3-serial setserial \
        usbutils v4l-utils udev && \
    rm -rf /var/lib/apt/lists/* && \
    pip3 install --no-cache-dir Jetson.GPIO

# ONNX Runtime C++ — snappy_computer_vision's CMakeLists requires it even with
# WITH_CUDA=OFF. Same version CI installs (1.17.3), aarch64 build.
RUN ORT_VER=1.17.3 && \
    curl -fL -o /tmp/ort.tgz \
        https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-aarch64-${ORT_VER}.tgz && \
    tar -xzf /tmp/ort.tgz -C /tmp && \
    cp -r /tmp/onnxruntime-linux-aarch64-${ORT_VER}/include/. /usr/local/include/ && \
    cp -r /tmp/onnxruntime-linux-aarch64-${ORT_VER}/lib/. /usr/local/lib/ && \
    ldconfig && \
    rm -rf /tmp/ort.tgz /tmp/onnxruntime-linux-aarch64-${ORT_VER}

RUN rosdep init 2>/dev/null || true

# Ask the NVIDIA runtime to mount the full Tegra driver/CUDA/multimedia stack.
ENV NVIDIA_VISIBLE_DEVICES=all \
    NVIDIA_DRIVER_CAPABILITIES=all

# Interactive shells come up with ROS and the workspace overlay sourced.
RUN echo 'source /opt/ros/humble/setup.bash' >> /root/.bashrc && \
    echo '[ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash' >> /root/.bashrc

WORKDIR /ros2_ws
CMD ["/bin/bash"]