# QGroundControl Quick Start for Developers

Fast-track guide to building and developing QGroundControl.

## Prerequisites

### Required Tools
- **Qt 6.10.0** (gcc_64, msvc2022_64, macos, or clang_64)
- **CMake 3.25+**
- **Ninja** build system
- **C++20** compiler (GCC 11+, Clang 14+, MSVC 2022+)
- **Git** with submodules

### Platform-Specific

**Ubuntu/Debian:**
```bash
sudo apt install build-essential ninja-build git cmake \
  libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  libsdl2-dev speech-dispatcher
```

**macOS:**
```bash
brew install cmake ninja gstreamer sdl2
xcode-select --install
```

**Windows:**
- Visual Studio 2022 (Desktop C++ workload)
- Install CMake and Ninja via installer or chocolatey

## 5-Minute Build

```bash
# 1. Clone repository with submodules
git clone --recursive https://github.com/mavlink/qgroundcontrol.git
cd qgroundcontrol

# 2. Configure (adjust Qt path)
~/Qt/6.10.0/gcc_64/bin/qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 3. Build
cmake --build build --config Debug -j$(nproc)

# 4. Run
./build/Debug/QGroundControl
```

## Development Workflow

### Build & Run
```bash
# Debug build with tests
qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DQGC_BUILD_TESTING=ON
cmake --build build -j$(nproc)
./build/Debug/QGroundControl

# Run unit tests
./build/Debug/QGroundControl --unittest

# Run specific test
./build/Debug/QGroundControl --unittest:FactSystemTest
```

### Code Formatting
```bash
# Install pre-commit hooks (one-time)
pip install pre-commit
pre-commit install

# Format all files
pre-commit run --all-files

# Format specific C++ file
clang-format -i src/Vehicle/Vehicle.cc
```

### Clean Rebuild
```bash
# Clean and rebuild
rm -rf build
qt-cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

### Documentation (VitePress)
```bash
# Install dependencies
npm install  # or yarn install

# Run dev server
npm run docs:dev

# Build docs
npm run docs:build
```

## Common Build Options

```bash
# All options in cmake/CustomOptions.cmake
-DQGC_STABLE_BUILD=ON            # Release build (vs daily)
-DQGC_BUILD_TESTING=ON           # Enable unit tests
-DQGC_ENABLE_BLUETOOTH=ON        # Bluetooth support
-DQGC_ENABLE_GST_VIDEOSTREAMING=ON  # GStreamer video
-DQGC_DISABLE_APM_PLUGIN=ON      # Disable ArduPilot
-DQGC_DISABLE_PX4_PLUGIN=ON      # Disable PX4
```

## IDE Setup

### VSCode
1. Install extensions:
   - C/C++ or clangd
   - CMake Tools
   - Qt tools (optional)
2. Open folder in VSCode
3. Select CMake preset from status bar
4. Build with F7, debug with F5

### Qt Creator
1. File → Open File or Project → CMakeLists.txt
2. Select Qt 6.10.0 kit
3. Build (Ctrl+B), Run (Ctrl+R)
4. Enable Clang Code Model in Options

### CLion
1. Open project folder
2. CLion auto-detects CMake configuration
3. Build (Ctrl+F9), Run (Shift+F10)
4. Configure Qt in Settings → Build → CMake

## Troubleshooting

**CMake can't find Qt:**
```bash
# Set Qt path explicitly
export Qt6_DIR=~/Qt/6.10.0/gcc_64
# Or use qt-cmake wrapper
~/Qt/6.10.0/gcc_64/bin/qt-cmake -B build
```

**Build fails with missing dependencies:**
```bash
# Update submodules
git submodule update --init --recursive
```

**Tests fail:**
```bash
# Run with verbose output
./build/Debug/QGroundControl --unittest -v2
```

**GStreamer errors:**
```bash
# Ubuntu: Install GStreamer dev packages
sudo apt install libgstreamer-plugins-{base,good,bad,ugly}1.0-dev
```

## Next Steps

- **Read**: [CONTRIBUTING.md](.github/CONTRIBUTING.md) - Coding guidelines
- **Read**: [copilot-instructions.md](.github/copilot-instructions.md) - Architecture patterns
- **Read**: [AGENTS.md](AGENTS.md) - Quick reference
- **Forum**: https://discuss.px4.io/c/qgroundcontrol
- **Discord**: https://discord.gg/dronecode

## Useful Commands Reference

```bash
# Build commands
cmake --build build                    # Build default target
cmake --build build --target clean     # Clean build
cmake --build build -j$(nproc)         # Parallel build

# Testing
./build/Debug/QGroundControl --unittest              # All tests
./build/Debug/QGroundControl --unittest:TestName     # Specific test
./build/Debug/QGroundControl --unittest -v2          # Verbose

# Code quality
clang-format -i src/**/*.{cc,h}       # Format all C++ files
clang-tidy src/Vehicle/Vehicle.cc     # Static analysis
cmake-format -i CMakeLists.txt        # Format CMake files

# Git workflow
git submodule update --init --recursive   # Initialize submodules
git submodule update --remote             # Update submodules
pre-commit run --all-files                # Run all pre-commit hooks
```

## Platform-Specific Notes

### Android
```bash
# Configure for Android (requires Android NDK)
~/Qt/6.10.0/android_arm64_v8a/bin/qt-cmake -B build-android \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a
cmake --build build-android
```

### iOS
```bash
# Configure for iOS (macOS only)
~/Qt/6.10.0/ios/bin/qt-cmake -B build-ios \
  -DCMAKE_SYSTEM_NAME=iOS
cmake --build build-ios
```

### Windows (Visual Studio)
```powershell
# Use Qt's CMake wrapper
C:\Qt\6.10.0\msvc2022_64\bin\qt-cmake.bat -B build -G Ninja
cmake --build build --config Debug
.\build\Debug\QGroundControl.exe
```

---

**Need help?** Check the [Developer Guide](https://dev.qgroundcontrol.com/) or ask on [Discord](https://discord.gg/dronecode).
