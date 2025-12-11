#!/bin/bash
set -euo pipefail

# Validate build type
VALID_BUILD_TYPES=("Release" "Debug" "RelWithDebInfo" "MinSizeRel")
if [[ ! " ${VALID_BUILD_TYPES[*]} " =~ .*\ ${BUILD_TYPE}\ .* ]]; then
    echo "ERROR: Invalid BUILD_TYPE: ${BUILD_TYPE}"
    echo "Valid types: ${VALID_BUILD_TYPES[*]}"
    exit 1
fi

# Configure
qt-cmake -S /project/source -B /project/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
cmake --build /project/build \
    --target all \
    --parallel "$(nproc)"

# Install
cmake --install /project/build \
    --config "${BUILD_TYPE}" \
    --strip

exec "$@"