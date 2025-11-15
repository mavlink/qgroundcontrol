# QGroundControl Custom Build Example

This example demonstrates how to create a white-label or customized version of QGroundControl for commercial or specialized use cases.

## What is a Custom Build?

A custom build allows you to:
- **Brand** - Replace logos, colors, and app name
- **Simplify** - Hide unused features for specific use cases
- **Customize** - Add custom widgets and workflows
- **Configure** - Override default settings
- **Restrict** - Lock down settings users shouldn't modify

![Custom Build Screenshot](README.jpg)

## Quick Start

### 1. Create Custom Directory
```bash
# Copy example to 'custom' directory
cp -r custom-example custom
cd custom
```

### 2. Customize Build
Edit `cmake/CustomOverrides.cmake`:
```cmake
# Branding
set(QGC_APP_NAME "MyDrone GCS" CACHE STRING "" FORCE)
set(QGC_ORG_NAME "MyCompany" CACHE STRING "" FORCE)
set(QGC_ORG_DOMAIN "mycompany.com" CACHE STRING "" FORCE)

# Disable unused autopilot
set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "" FORCE)
```

### 3. Build
```bash
cd ..  # Return to repository root
qt-cmake -B build -G Ninja
cmake --build build
```

Your custom build will use branding and settings from `custom/` directory.

## Example Features

This example demonstrates:

### 1. Off-The-Shelf Vehicle Assumption
- **Hidden Setup**: Most vehicle setup is hidden since it's pre-configured
- **Simpler UI**: Fewer options for end users
- **Advanced Mode**: Full QGC experience still available for experts

### 2. Custom Branding
- **Logo**: Custom splash screen and toolbar icons
- **Colors**: Corporate color palette throughout UI
- **Name**: Custom application name and organization

### 3. Custom Instruments
- **Custom Widget**: Replaces standard QGC instrument panel
- **Specialized Display**: Tailored to specific vehicle type

### 4. Application Settings Override
- **Hidden Settings**: Users can't modify critical settings
- **Changed Defaults**: Better defaults for your use case
- **Locked Configuration**: Prevent accidental misconfiguration

## Directory Structure

```
custom/
├── CMakeLists.txt          # Custom build configuration
├── custom.qrc              # Custom resources (images, QML)
├── cmake/
│   └── CustomOverrides.cmake  # Build option overrides
├── src/
│   ├── CustomPlugin.{cc,h}     # Custom plugin implementation
│   └── CustomFirmwarePlugin.{cc,h}  # Firmware customization
├── res/
│   ├── Custom.qml          # Custom UI components
│   └── images/             # Custom branding assets
├── android/                # Android-specific customization
└── deploy/                 # Deployment scripts

```

## Customization Guide

### Branding

#### Change App Name and Organization
Edit `cmake/CustomOverrides.cmake`:
```cmake
set(QGC_APP_NAME "MyDrone GCS" CACHE STRING "" FORCE)
set(QGC_ORG_NAME "MyCompany" CACHE STRING "" FORCE)
set(QGC_ORG_DOMAIN "mycompany.com" CACHE STRING "" FORCE)
set(QGC_PACKAGE_NAME "com.mycompany.mygcs" CACHE STRING "" FORCE)
```

#### Replace Icons and Images
1. Add your images to `res/images/`
2. Update `custom.qrc` to include new resources
3. Reference in QML: `source: "qrc:/custom/MyLogo.png"`

#### Change Color Palette
Edit QML files in `res/` and override `QGCPalette` colors:
```qml
QGCPalette {
    colorGroupEnabled: true
    colorBackground: "#1e1e1e"      // Dark background
    colorText: "#00aaff"             // Branded blue
    // ... more colors
}
```

### Hide/Show Features

#### Hide Autopilot Plugin
```cmake
set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "" FORCE)
set(QGC_DISABLE_PX4_PLUGIN ON CACHE BOOL "" FORCE)
```

#### Hide Video Streaming
```cmake
set(QGC_ENABLE_GST_VIDEOSTREAMING OFF CACHE BOOL "" FORCE)
set(QGC_ENABLE_UVC OFF CACHE BOOL "" FORCE)
```

#### Disable Communication Links
```cmake
set(QGC_ENABLE_BLUETOOTH OFF CACHE BOOL "" FORCE)
set(QGC_NO_SERIAL_LINK ON CACHE BOOL "" FORCE)
```

### Custom Settings Defaults

Override settings in `src/CustomPlugin.cc`:
```cpp
void CustomPlugin::setCustomDefaultSettings() {
    // Override default units
    appSettings()->distanceUnits()->setRawValue(DistanceUnits::Meters);

    // Set default map provider
    appSettings()->mapProvider()->setRawValue("Mapbox");

    // Hide telemetry log replay
    appSettings()->telemetryLogReplay()->setRawValue(false);
}
```

### Add Custom UI Components

Create custom QML widgets in `res/`:
```qml
// res/CustomInstrumentPanel.qml
import QtQuick 2.15
import QGroundControl 1.0

Rectangle {
    id: customPanel

    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle

    // Custom instrument display
    Column {
        Text { text: "Altitude: " + vehicle.altitude.value.toFixed(1) + "m" }
        Text { text: "Speed: " + vehicle.groundSpeed.value.toFixed(1) + "m/s" }
        // ... more custom instruments
    }
}
```

Reference in main UI override.

### Platform-Specific Customization

#### Android
- Customize `android/AndroidManifest.xml`
- Add custom icons in `android/res/`
- Override package name and permissions

#### iOS
- Customize `ios/Info.plist`
- Add custom icons in `ios/Assets.xcassets/`

#### Windows
- Customize installer in `deploy/windows/`
- Add custom exe icon

#### macOS
- Customize DMG in `deploy/macos/`
- Add custom app icon

## Build Variants

### Minimal Build (Embedded Devices)
```cmake
set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "" FORCE)
set(QGC_ENABLE_BLUETOOTH OFF CACHE BOOL "" FORCE)
set(QGC_ENABLE_GST_VIDEOSTREAMING OFF CACHE BOOL "" FORCE)
set(QGC_VIEWER3D OFF CACHE BOOL "" FORCE)
```

### PX4-Only Build
```cmake
set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "" FORCE)
```

### ArduPilot-Only Build
```cmake
set(QGC_DISABLE_PX4_PLUGIN ON CACHE BOOL "" FORCE)
```

## Testing Custom Build

### Verify Branding
```bash
# Check app name
./build/Debug/QGroundControl --version

# Check about dialog
./build/Debug/QGroundControl
# Help → About
```

### Test Settings Overrides
```bash
# Settings stored in ~/.config/MyCompany/MyDrone GCS.conf
cat ~/.config/MyCompany/"MyDrone GCS.conf"
```

### Verify Hidden Features
Check that disabled plugins/features don't appear in UI.

## Distribution

### Create Installer

**Linux (AppImage)**:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DQGC_STABLE_BUILD=ON
cmake --build build
cmake --install build
# Creates AppImage in deploy output
```

**Windows (EXE)**:
```bash
# Build with installer
cmake --build build --target package
```

**macOS (DMG)**:
```bash
# Build with installer and notarization
cmake --install build
```

**Android (APK)**:
```bash
# Build Android package
cmake --build build-android
```

## Advanced Customization

### Custom Firmware Plugin
Implement `CustomFirmwarePlugin` to:
- Override flight modes
- Customize parameter metadata
- Add custom MAVLink handling
- Modify setup UI components

See `src/CustomFirmwarePlugin.cc` for example.

### Custom AutoPilot Plugin
Implement `CustomAutoPilotPlugin` to:
- Add custom setup pages
- Override vehicle components
- Customize summary page

### Inject Custom Code
Use QGC's plugin system to inject functionality without modifying core:
```cpp
class CustomPlugin : public QGCCorePlugin {
    Q_OBJECT
public:
    // Override virtual methods
    virtual QString brandImageIndoor() const override;
    virtual QString brandImageOutdoor() const override;
    virtual QGCOptions* options() override;
};
```

## Source Code Comments

The example source code is **fully commented** to explain:
- What each customization does
- Why it's done that way
- How to modify for your use case

**Read the source files** in `src/` for detailed explanations.

## Troubleshooting

**Custom build not picking up changes:**
```bash
# Clean rebuild
rm -rf build
cmake -B build
cmake --build build
```

**Resources not loading:**
```bash
# Verify .qrc file syntax
# Check resource paths are correct
```

**Settings not overridden:**
```bash
# Clear settings
rm ~/.config/MyCompany/"MyDrone GCS.conf"
# Restart app
```

**Icons/images not showing:**
```bash
# Verify resources are in custom.qrc
# Check QML uses correct qrc:/ path
```

## See Also

- **[QGC Dev Guide - Custom Builds](https://dev.qgroundcontrol.com/en/custom_build/custom_build.html)** - Complete documentation
- **[cmake/README.md](../cmake/README.md)** - Build options reference
- **[QUICKSTART.md](../QUICKSTART.md)** - Build instructions
- **[Custom Build Forum](https://discuss.px4.io/c/qgroundcontrol/qgc-custom-build)** - Community support

## License

Custom builds must comply with QGroundControl's dual license (Apache 2.0 / GPL v3).

See [COPYING.md](../.github/COPYING.md) for license details.

---

**Happy Customizing!** 🚁

For questions or help, visit the [QGroundControl Forum](https://discuss.px4.io/c/qgroundcontrol) or [Discord](https://discord.gg/dronecode).
