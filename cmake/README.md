# CMake Build Configuration

Documentation for QGroundControl's CMake build system and options.

## Directory Structure

```
cmake/
├── CustomOptions.cmake      # All build configuration options
├── Helpers.cmake            # CMake utility functions
├── PrintSummary.cmake       # Build configuration summary
├── Toolchain.cmake          # Compiler and platform toolchain setup
├── find-modules/            # Custom CMake find modules
├── install/                 # Installation and packaging scripts
├── modules/                 # CMake modules (GStreamer, etc.)
├── platform/                # Platform-specific configurations
└── presets/                 # CMake presets per platform
```

## Build Options Reference

All options defined in `CustomOptions.cmake` can be set via CMake command line:
```bash
cmake -B build -DOPTION_NAME=ON
```

### Application Metadata

| Option | Default | Description |
|--------|---------|-------------|
| `QGC_APP_NAME` | "QGroundControl" | Application display name |
| `QGC_APP_COPYRIGHT` | "Copyright (c) 2025..." | Copyright notice |
| `QGC_ORG_NAME` | "QGroundControl" | Organization name |
| `QGC_ORG_DOMAIN` | "qgroundcontrol.com" | Organization domain |
| `QGC_PACKAGE_NAME` | "org.mavlink.qgroundcontrol" | Package identifier |
| `QGC_SETTINGS_VERSION` | "9" | Settings schema version |

### Build Configuration

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | OFF | Build using shared libraries |
| `QGC_STABLE_BUILD` | OFF | Stable release (vs daily build) |
| `QGC_USE_CACHE` | ON | Enable ccache/sccache |
| `QGC_BUILD_INSTALLER` | ON | Build platform installers |
| `QGC_BUILD_TESTING` | ON (Debug) | Enable unit tests |
| `QGC_DEBUG_QML` | ON (Debug) | Enable QML debugging |

### Feature Flags

| Option | Default | Description |
|--------|---------|-------------|
| `QGC_UTM_ADAPTER` | OFF | UTM (Unmanned Traffic Management) Adapter |
| `QGC_VIEWER3D` | ON | 3D Viewer (requires Qt Quick 3D) |

### Communication Options

| Option | Default | Description |
|--------|---------|-------------|
| `QGC_ENABLE_BLUETOOTH` | ON | Bluetooth communication links |
| `QGC_ZEROCONF_ENABLED` | OFF | ZeroConf/Bonjour discovery |
| `QGC_AIRLINK_DISABLED` | ON | Disable AIRLink support |
| `QGC_NO_SERIAL_LINK` | OFF | Disable serial port communication |

### Video Streaming

| Option | Default | Description |
|--------|---------|-------------|
| `QGC_ENABLE_UVC` | ON | USB Video Class device support |
| `QGC_ENABLE_GST_VIDEOSTREAMING` | ON | GStreamer video backend |
| `QGC_CUSTOM_GST_PACKAGE` | OFF | Use QGC GStreamer packages |
| `QGC_ENABLE_QT_VIDEOSTREAMING` | OFF | QtMultimedia video backend |

### Autopilot Plugins

| Option | Default | Description |
|--------|---------|-------------|
| `QGC_DISABLE_APM_MAVLINK` | OFF | Disable ArduPilot MAVLink dialect |
| `QGC_DISABLE_APM_PLUGIN` | OFF | Disable ArduPilot plugin |
| `QGC_DISABLE_PX4_PLUGIN` | OFF | Disable PX4 plugin |

### Platform-Specific

**Android:**
- `QGC_QT_ANDROID_COMPILE_SDK_VERSION` = 35
- `QGC_QT_ANDROID_TARGET_SDK_VERSION` = 35
- `QGC_QT_ANDROID_MIN_SDK_VERSION` = 28

## Common Build Configurations

### Standard Debug Build
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DQGC_BUILD_TESTING=ON
cmake --build build
```

### Release Build with Installer
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQGC_STABLE_BUILD=ON \
  -DQGC_BUILD_INSTALLER=ON
cmake --build build
cmake --install build
```

### Minimal Build (No Autopilots)
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DQGC_DISABLE_APM_PLUGIN=ON \
  -DQGC_DISABLE_PX4_PLUGIN=ON
```

### PX4-Only Build
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQGC_DISABLE_APM_PLUGIN=ON
```

### ArduPilot-Only Build
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQGC_DISABLE_PX4_PLUGIN=ON
```

### Video Streaming Disabled
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DQGC_ENABLE_GST_VIDEOSTREAMING=OFF \
  -DQGC_ENABLE_UVC=OFF
```

### Embedded/Minimal Build
```bash
qt-cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DQGC_ENABLE_BLUETOOTH=OFF \
  -DQGC_ENABLE_GST_VIDEOSTREAMING=OFF \
  -DQGC_VIEWER3D=OFF
```

## Custom Builds

Create a custom build by:

1. **Copy custom-example**:
   ```bash
   cp -r custom-example custom
   cd custom
   ```

2. **Edit `cmake/CustomOverrides.cmake`**:
   ```cmake
   # Override default options
   set(QGC_APP_NAME "MyDrone GCS" CACHE STRING "" FORCE)
   set(QGC_ORG_NAME "MyCompany" CACHE STRING "" FORCE)
   set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "" FORCE)
   ```

3. **Build**:
   ```bash
   qt-cmake -B build -G Ninja
   cmake --build build
   ```

See [custom-example/README.md](../custom-example/README.md) for details.

## CMake Presets

CMake presets are available in `cmake/presets/` for each platform:
- `common.json` - Shared configuration
- `Linux.json` - Linux-specific presets
- `macOS.json` - macOS-specific presets
- `Windows.json` - Windows-specific presets
- `Android.json` - Android-specific presets
- `iOS.json` - iOS-specific presets

**Usage**:
```bash
# List available presets
cmake --list-presets

# Use a preset
cmake --preset=linux-debug
cmake --build --preset=linux-debug
```

## Helper Functions (Helpers.cmake)

Custom CMake functions available:

- `qgc_add_module()` - Add a QGC module with proper dependencies
- `qgc_install_plugin()` - Install plugin files
- `qgc_add_test()` - Add unit test

See `Helpers.cmake` for implementation details.

## Build Summary

After configuration, a summary is printed showing:
- Platform and architecture
- Build type and options
- Enabled features
- Autopilot plugins
- Video streaming backends
- Installation targets

Example:
```
========================================
QGroundControl Build Summary
========================================
Platform: Linux (x86_64)
Build Type: Debug
Qt Version: 6.10.0
========================================
Features:
  ✓ Unit Tests
  ✓ QML Debugging
  ✓ Bluetooth
  ✓ GStreamer Video
  ✓ 3D Viewer
========================================
Autopilots:
  ✓ PX4 Plugin
  ✓ ArduPilot Plugin
========================================
```

## Troubleshooting

**Qt not found:**
```bash
# Use qt-cmake wrapper
~/Qt/6.10.0/gcc_64/bin/qt-cmake -B build
# Or set Qt6_DIR
cmake -B build -DQt6_DIR=~/Qt/6.10.0/gcc_64/lib/cmake/Qt6
```

**GStreamer not found:**
```bash
# Ubuntu/Debian
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```

**ccache not working:**
```bash
# Disable caching
cmake -B build -DQGC_USE_CACHE=OFF
```

**Clear CMake cache:**
```bash
rm -rf build
cmake -B build
```

---

**See also:**
- [QUICKSTART.md](../QUICKSTART.md) - Quick build guide
- [CustomOptions.cmake](CustomOptions.cmake) - All options source
- [Dev Guide](https://dev.qgroundcontrol.com/) - Complete documentation
