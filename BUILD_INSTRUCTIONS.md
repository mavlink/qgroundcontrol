# JIACDIGCS Build Instructions

## Overview

JIACDIGCS (Professional Multi-UAV Swarm Command and Control Platform) is a complete rebranding and feature extension of QGroundControl, supporting Windows, Android, and iOS platforms only.

## Supported Platforms

| Platform | Minimum Version | Build Status |
|----------|----------------|---------------|
| Windows | Windows 10+ | ✅ Supported |
| Android | Android 9 (API 28)+ | ✅ Supported |
| iOS | iOS 14.0+ | ✅ Supported |
| Linux | N/A | ❌ Removed |

## Prerequisites

### Common Requirements
- CMake 3.25+
- Git

### Windows Build
- Visual Studio 2022 with C++ toolset
- Qt 6.10+ (from qt.io)
- NSIS for installer creation
- Windows SDK

### Android Build
- Android Studio or command-line Android SDK
- Android NDK r27c (from build-config.json)
- Java 17+
- Gradle 8.x

### iOS Build
- Xcode 16.x+
- macOS 13.0+
- CocoaPods
- Qt for iOS (from qt.io)

## Quick Start

### 1. Clone Repository
```bash
git clone https://github.com/your-org/jiacdigcs.git
cd jiacdigcs
```

### 2. Configure Build

**Windows (Visual Studio)**
```bash
cmake -B build -G "Visual Studio 17 2022" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DQGC_APP_NAME="JIACDIGCS"
```

**Android**
```bash
cmake -B build -G "Unix Makefiles" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT ^
    -DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT
```

**iOS**
```bash
cmake -B build -G "Xcode" ^
    -DPLATFORM=IOS ^
    -DCMAKE_BUILD_TYPE=Release
```

### 3. Build

**Windows**
```bash
cmake --build build --config Release
```

**Android**
```bash
cmake --build build --target android_build
```

**iOS**
```bash
cmake --build build --config Release
```

## Build Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `QGC_APP_NAME` | Application name | JIACDIGCS |
| `QGC_ORG_NAME` | Organization name | JIACDIGCS |
| `QGC_PACKAGE_NAME` | Package identifier | org.jiacdigcs.swarm |
| `QGC_STABLE_BUILD` | Stable build mode | OFF |
| `QGC_BUILD_TESTING` | Enable unit tests | ON (Debug) |
| `QGC_ENABLE_COVERAGE` | Code coverage | OFF |

## Swarm Features

### Enabling Swarm Mode
Swarm mode is enabled via the SwarmManager singleton:
```cpp
SwarmManager::instance()->setSwarmEnabled(true);
```

### Supported Formations
- Line
- V Formation
- Grid
- Circle
- Custom (user-defined)

### Swarm Commands
- `synchronizedTakeoff(altitude)` - All vehicles takeoff
- `synchronizedLand()` - All vehicles land
- `synchronizedRTL()` - All vehicles return to home
- `emergencyStopAll()` - Emergency stop all vehicles
- `executeFormationFlight()` - Start formation flight
- `holdPosition()` - Hold position for all vehicles

### Multi-Vehicle Support
- Support for up to 20 simultaneous UAVs
- Real-time telemetry for all swarm members
- Leader-follower mode
- Formation coordination

## Architecture Overview

### Core Modules

```
src/
├── Swarm/               # Swarm management module
│   ├── SwarmManager     # Central swarm coordination
│   └── QmlControls/     # Swarm UI components
├── Vehicle/            # Vehicle management
│   ├── MultiVehicleManager  # Multi-vehicle coordination
│   └── Vehicle.cc/h    # Individual vehicle class
├── MissionManager/     # Mission planning/execution
├── MAVLink/           # MAVLink protocol handling
└── MainWindow/        # Main UI framework
```

### SwarmManager Class
The `SwarmManager` class provides:
- Singleton access for swarm-wide operations
- Vehicle registration/management
- Formation coordination
- Synchronized command execution
- Health monitoring and alerts

### QML Components

| Component | Purpose |
|-----------|---------|
| `SwarmInterface.qml` | Main swarm dashboard |
| `SwarmControlPanel.qml` | Swarm command controls |
| `SwarmVehicleStatus.qml` | Individual vehicle status |
| `SwarmTelemetryWidget.qml` | Telemetry visualization |
| `SwarmAlertSystem.qml` | Alert management |

## Deployment

### Android APK
```bash
cd build
make package
# APK located at: build/android-build/build/outputs/apk/
```

### Windows Installer
```bash
cmake --install build --config Release
# Creates NSIS installer
```

### iOS App
```bash
xcodebuild -workspace build/ios/JIACDIGCS.xcworkspace \
    -scheme JIACDIGCS -configuration Release \
    -archivePath build/JIACDIGCS.xcarchive
xcodebuild -exportArchive -archivePath build/JIACDIGCS.xcarchive \
    -exportOptionsPlist ios/ExportOptions.plist \
    -exportPath build/ios
```

## Troubleshooting

### Common Issues

1. **CMake can't find Qt**
   - Ensure Qt is installed and `Qt6_DIR` is set correctly
   - Use Qt Online Installer from qt.io

2. **Android build fails with NDK error**
   - Verify NDK version matches build-config.json
   - Set `ANDROID_NDK_ROOT` environment variable

3. **iOS build fails on code signing**
   - Ensure valid provisioning profiles are installed
   - Set code signing identity in Xcode

## Further Documentation

- [AGENTS.md](AGENTS.md) - Developer documentation
- [CODING_STYLE.md](CODING_STYLE.md) - Code standards
- [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) - Contribution guidelines