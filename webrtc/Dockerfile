# STAGE 1: Base setup
ARG ARCH=amd64
ARG ROS_DISTRO=humble
FROM ${ARCH}/ros:${ROS_DISTRO}-ros-base-jammy AS base

# Declare the arguments again after the FROM statement
ARG ARCH
ARG ROS_DISTRO
ARG BUILD_TYPE=Release

# Add architecture if it's arm64v8
RUN if [ "${ARCH}" = "arm64v8" ]; then dpkg --add-architecture arm64; fi
# Update and install necessary packages
RUN apt update && apt install python3-pip curl wget zip unzip net-tools iputils-ping libssl-dev build-essential htop tar ninja-build ros-${ROS_DISTRO}-test-msgs ros-${ROS_DISTRO}-example-interfaces ros-${ROS_DISTRO}-topic-tools valgrind -yq
# Install build dependencies
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y \
    build-essential git sudo vim libprotobuf-dev protobuf-compiler locales wget curl cmake pkg-config ca-certificates \
    ninja-build  libssl-dev libboost-all-dev libglib2.0-dev libgtk-3-dev libpulse-dev libasound2-dev patch

# Install cross-compiler
RUN apt install -y g++-aarch64-linux-gnu gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf clang

# Install numpy
RUN pip install --target=/usr/local/lib/python3.10/dist-packages numpy

ARG CMAKE_VERSION=3.30.1

# Download and install the latest CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}.tar.gz \
    && tar -zxvf cmake-${CMAKE_VERSION}.tar.gz \
    && cd cmake-${CMAKE_VERSION} \
    && ./bootstrap \
    && make \
    && make install \
    && cd .. \
    && rm -rf cmake-${CMAKE_VERSION}*

# Set environment variables to prioritize the new CMake
ENV PATH="/usr/local/bin:$PATH"

# source ros and add to bashrc
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc

# Create a new ws for ROS
RUN mkdir -p /home/webrtc_ws/src