# QGC Qt Creator C++ Plugin

Native Qt Creator plugin for QGroundControl development.

## Features

- **FactGroup Wizard**: File → New → QGroundControl → FactGroup
- **MAVLink Completion**: Autocomplete for `MAVLINK_MSG_ID_*` and `mavlink_msg_*` functions
- **Menu Integration**: Tools → QGroundControl submenu
- **Future**: Hover documentation, vehicle null-check analysis

## Requirements

- Qt Creator 14.0+ with Plugin Development component installed
- Qt 6.6+ (matching your Qt Creator version)
- CMake 3.16+

## Building

### 1. Install Qt Creator Plugin SDK

When installing Qt Creator, select the **Plugin Development** component.

Or build Qt Creator from source:
```bash
git clone https://github.com/qt-creator/qt-creator.git
cd qt-creator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 2. Configure the Plugin

```bash
cd tools/qtcreator/plugin

# Point to Qt Creator and Qt installations
cmake -B build \
    -DCMAKE_PREFIX_PATH="/path/to/qtcreator;/path/to/Qt/6.6.0/gcc_64" \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

### 3. Install the Plugin

```bash
# Linux
cp build/lib/qtcreator/plugins/libQGCPlugin.so ~/.local/share/QtProject/qtcreator/plugins/

# macOS
cp build/lib/qtcreator/plugins/libQGCPlugin.dylib ~/Library/Application\ Support/QtProject/qtcreator/plugins/

# Windows
copy build\lib\qtcreator\plugins\QGCPlugin.dll %APPDATA%\QtProject\qtcreator\plugins\
```

### 4. Run Qt Creator

```bash
# Or use the RunQtCreator target for testing
cmake --build build --target RunQtCreator
```

## Directory Structure

```
cpp/
├── CMakeLists.txt           # Build configuration
├── QGCPlugin.json.in        # Plugin metadata template
├── README.md
└── src/
    ├── qgcplugin.h/cpp      # Main plugin class (IPlugin)
    ├── qgcconstants.h       # Plugin constants
    ├── qgcplugin_global.h   # Export macros
    ├── wizard/              # FactGroup wizard
    │   ├── factgroupwizardfactory.h/cpp
    │   └── factgroupwizardpage.h/cpp
    ├── completion/          # Code completion providers
    │   └── mavlinkcompletionassist.h/cpp  # MAVLink autocomplete
    └── analysis/            # Static analysis (planned)
```

## MAVLink Completion

The plugin provides intelligent autocomplete for MAVLink code patterns:

### Message ID Constants
When typing `MAVLINK_MSG_ID_` or in a `switch(message.msgid)` context:
- Shows all common MAVLink message IDs
- Displays message ID number and description
- Example: `MAVLINK_MSG_ID_HEARTBEAT` → `[0] Vehicle heartbeat - basic status`

### Decode Functions
When typing `mavlink_msg_`:
- Shows decode functions: `mavlink_msg_heartbeat_decode`
- Shows field getters: `mavlink_msg_heartbeat_get_type`, `mavlink_msg_heartbeat_get_autopilot`

### Covered Messages
Common MAVLink messages included:
- System: HEARTBEAT, SYS_STATUS, SYSTEM_TIME, PING
- Position: ATTITUDE, LOCAL_POSITION_NED, GLOBAL_POSITION_INT, GPS_RAW_INT
- Navigation: NAV_CONTROLLER_OUTPUT, MISSION_CURRENT, MISSION_ITEM_REACHED
- RC/Control: RC_CHANNELS, RC_CHANNELS_OVERRIDE, SERVO_OUTPUT_RAW
- Parameters: PARAM_VALUE, PARAM_SET
- Commands: COMMAND_LONG, COMMAND_ACK
- Telemetry: VFR_HUD, ALTITUDE, BATTERY_STATUS
- Sensors: SCALED_IMU, SCALED_PRESSURE, WIND
- Status: STATUSTEXT, EXTENDED_SYS_STATE
- High Latency: HIGH_LATENCY, HIGH_LATENCY2

## Development

### Adding New Features

1. **New Wizard**: Implement `Core::BaseFileWizardFactory`
2. **New Action**: Register via `Core::ActionManager`
3. **Completion**: Implement `TextEditor::CompletionAssistProvider`
4. **Analysis**: Use `TextEditor::TextDocument` for real-time checks

### Debugging

1. Set `QT_DEBUG_PLUGINS=1` environment variable
2. Check Qt Creator's plugin loading in Help → About Plugins
3. Use the RunQtCreator target for quick iteration

### References

- [Qt Creator Plugin Development](https://doc.qt.io/qtcreator-extending/first-plugin.html)
- [QodeAssist Plugin](https://github.com/Palm1r/QodeAssist) - AI assistant example
- [ROS Qt Creator Plugin](https://github.com/ros-industrial/ros_qtc_plugin) - Project manager example

## Comparison with Lua Extension

| Feature | Lua Extension | C++ Plugin |
|---------|---------------|------------|
| Installation | Copy files | Build + install |
| Qt Creator version | 14+ only | Must match exactly |
| Performance | Good | Best |
| Capabilities | Limited API | Full API access |
| Wizard UI | Basic dialogs | Native Qt widgets |
| LSP integration | Via subprocess | Could be native |

**Recommendation**: Use the Lua extension for most features. Use the C++ plugin only for features requiring deep IDE integration (custom wizards, completion providers).
