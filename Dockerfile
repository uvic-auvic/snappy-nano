FROM nil2thrill/snappy-ros:humble

# Install vcs and any extra tools not already in your image
RUN apt-get update && apt-get install -y \
    python3-vcstool \
    python3-colcon-common-extensions \
    && rm -rf /var/lib/apt/lists/*
