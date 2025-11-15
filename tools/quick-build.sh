#!/bin/bash
# Quick build script for QGroundControl
# Automatically detects Qt installation and builds

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}QGroundControl Quick Build${NC}"
echo "================================"
echo ""

# Detect OS
OS="unknown"
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    NPROC="$(nproc)"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    NPROC="$(sysctl -n hw.ncpu)"
else
    echo -e "${RED}Unsupported OS: $OSTYPE${NC}"
    exit 1
fi

# Find Qt installation
QT_CMAKE=""
if [ "$OS" = "linux" ]; then
    QT_PATHS=(
        "$HOME/Qt/6.10.0/gcc_64/bin/qt-cmake"
        "/opt/Qt/6.10.0/gcc_64/bin/qt-cmake"
    )
elif [ "$OS" = "macos" ]; then
    QT_PATHS=(
        "$HOME/Qt/6.10.0/macos/bin/qt-cmake"
        "$HOME/Qt/6.10.0/clang_64/bin/qt-cmake"
    )
fi

for path in "${QT_PATHS[@]}"; do
    if [ -x "$path" ]; then
        QT_CMAKE="$path"
        break
    fi
done

if [ -z "$QT_CMAKE" ]; then
    echo -e "${RED}ERROR: Qt 6.10.0 not found${NC}"
    echo "Install from: https://www.qt.io/download-qt-installer"
    exit 1
fi

echo -e "${GREEN}✓${NC} Found Qt: $QT_CMAKE"

# Parse arguments
BUILD_TYPE="Debug"
BUILD_DIR="build"
CLEAN_BUILD=false
RUN_TESTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--release)
            BUILD_TYPE="Release"
            BUILD_DIR="build-release"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -r, --release    Build Release instead of Debug"
            echo "  -c, --clean      Clean build directory first"
            echo "  -t, --test       Run unit tests after build"
            echo "  -h, --help       Show this help message"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

echo -e "${GREEN}✓${NC} Build type: $BUILD_TYPE"
echo -e "${GREEN}✓${NC} Build directory: $BUILD_DIR"
echo ""

# Clean build if requested
if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Configure
if [ ! -d "$BUILD_DIR" ]; then
    echo "Configuring..."
    if [ "$RUN_TESTS" = true ]; then
        "$QT_CMAKE" -B "$BUILD_DIR" -G Ninja \
            -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
            -DQGC_BUILD_TESTING=ON
    else
        "$QT_CMAKE" -B "$BUILD_DIR" -G Ninja \
            -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    fi
    echo -e "${GREEN}✓${NC} Configuration complete"
    echo ""
fi

# Build
echo "Building with $NPROC cores..."
START_TIME=$(date +%s)
if cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$NPROC"; then
    END_TIME=$(date +%s)
    BUILD_TIME=$((END_TIME - START_TIME))
    echo ""
    echo -e "${GREEN}✓ Build successful${NC} (${BUILD_TIME}s)"
else
    echo ""
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    echo ""
    echo "Running unit tests..."
    if "./$BUILD_DIR/$BUILD_TYPE/QGroundControl" --unittest; then
        echo -e "${GREEN}✓ All tests passed${NC}"
    else
        echo -e "${RED}✗ Some tests failed${NC}"
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}Build complete!${NC}"
echo "Run: ./$BUILD_DIR/$BUILD_TYPE/QGroundControl"
