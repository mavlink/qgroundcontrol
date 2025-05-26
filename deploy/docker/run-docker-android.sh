#!/usr/bin/env bash

set -e

# Build the Docker image for Android
docker build --file ./deploy/docker/Dockerfile-build-android -t qgc-android-docker .

# Run the Docker container with adjusted mount points
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/workspace/build qgc-android-docker

# Add Git safe directory setting
# docker run --rm qgc-android-docker git config --global --add safe.directory /project/source

# docker run --cap-add SYS_ADMIN --device /dev/fuse --security-opt apparmor:unconfined --rm \
#     -v ${PWD}:/project/source \
#     -v ${PWD}/build:/project/build \
#     -e ANDROID_SDK_ROOT=/opt/android-sdk \
#     -e ANDROID_SDK_BUILD_TOOLS=/opt/android-sdk/build-tools/31.0.0 \
#     qgc-android-docker \
#     cmake -S /project/source -B /project/build \
#     -G Ninja \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DCMAKE_TOOLCHAIN_FILE=/opt/android-sdk/ndk/23.1.7779620/build/cmake/android.toolchain.cmake \
#     -DANDROID_ABI=arm64-v8a \
#     -DANDROID_PLATFORM=android-31 \
#     -DANDROID_BUILD_TOOLS=/opt/android-sdk/build-tools/31.0.0 \
#     -DCMAKE_CXX_STANDARD=17 \
#     -DQT_HOST_PATH_CMAKE_DIR=/opt/Qt/6.6.3/gcc_64/lib/cmake \
#     -DQT_HOST_PATH=/opt/Qt/6.6.3/gcc_64 \
#     -DQt6_DIR=/opt/Qt/6.6.3/android_arm64_v8a/lib/cmake/Qt6




# # Build and install the project
# docker run --cap-add SYS_ADMIN --device /dev/fuse --security-opt apparmor:unconfined --rm \
#     -v ${PWD}:/project/source \
#     -v ${PWD}/build:/project/build \
#     qgc-android-docker \
#     cmake --build /project/build --target all --config Release

# docker run --cap-add SYS_ADMIN --device /dev/fuse --security-opt apparmor:unconfined --rm \
#     -v ${PWD}:/project/source \
#     -v ${PWD}/build:/project/build \
#     qgc-android-docker \
#     cmake --install /project/build --config Release
