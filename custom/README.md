# Al-Bayraq Ground Control Station

Custom QGroundControl fork with object tracking capabilities for Al-Bayraq UAV operations.

## Quick Start

### First Build
```bash
# 1. Configure and build (takes ~20 minutes first time)
cmake -S . -B build   -DCMAKE_BUILD_TYPE=Release   -DQGC_UNITTEST_BUILD=OFF   -DQGC_VIEWER3D=ON   -DQT_QML_DEBUG=OFF

cmake --build build -- -j2

# 2. Run
./build/Release/AlBayraqGCS              # Linux/macOS
# build\Release\AlBayraqGCS.exe   # Windows
```

### After Code Changes
```bash
# Quick rebuild
cmake --build build -- -j2
```


## Connecting USb to WSl
& "C:\Program Files\usbipd-win\usbipd.exe" bind --busid 5-1
& "C:\Program Files\usbipd-win\usbipd.exe" attach --busid 5-1 --wsl




## Object Tracking Feature

**Access**: Analyze Tools → Object Tracking

Features:
- Start/Stop tracking controls
- Emergency stop button
- Auto follow and video recording
- System status indicators

## Key Files

- **Object Tracking UI**: `custom/res/Custom/ObjectTrackingDashboard.qml`
- **Plugin Code**: `custom/src/CustomPlugin.cc`
- **Resources**: `custom/res/custom.qrc`

---

**Al-Bayraq UAV Systems**
