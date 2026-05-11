# QGroundControl Guide (Daily Builds)

[![Discuss](https://img.shields.io/badge/discuss-px4-ff69b4.svg)](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-usage)
[![Discuss](https://img.shields.io/badge/discuss-ardupilot-ff69b4.svg)](http://discuss.ardupilot.org/c/ground-control-software/qgroundcontrol)

_QGroundControl_ provides full flight control and vehicle setup for PX4 or ArduPilot powered vehicles.
It provides easy and straightforward usage for beginners, while still delivering high end feature support for experienced users.

**Key Features:**

- Full setup/configuration of ArduPilot and PX4 Pro powered vehicles.
- Flight support for vehicles running PX4 and ArduPilot (or any other autopilot that communicates using the MAVLink protocol).
- Mission planning for autonomous flight.
- Flight map display showing vehicle position, flight track, waypoints and vehicle instruments.
- 3D viewer visualizing the 3D map of the environment (.osm file), the 3D model of the vehicle (only multi-rotors for the moment), and the mission 3D trajectory (including the waypoints).
- Video streaming with instrument display overlays.
- Support for managing multiple vehicles.
- QGC runs on Windows, OS X, Linux platforms, iOS and Android devices.

![](../../assets/quickstart/connected_vehicle.jpg)

::: info
This guide is an active work in progress.
The information provided should be correct, but you may find missing information or incomplete pages.
:::

::: tip
Information about _QGroundControl_ development, architecture, contributing, and translating can be found in the [Developer Guide](../qgc-dev-guide/index.md) section.
:::

## What's New Since Stable V5.0

The daily build adds the following features compared to the Stable V5.0 release.

### Log Viewer (Analyze View)

The [Log Viewer](analyze_view/log_viewer.md) (**Analyze > Log Viewer**) is a new unified post-flight analysis tool that supports three log formats:

- **ArduPilot DataFlash** (`.bin` / `.log`) — field charting, GPS flight path map, parameter inspection, and status messages.
- **PX4 ULog** (`.ulg`) — same capabilities using PX4 log format.
- **MAVLink Telemetry** (`.tlog`) — playback with speed control, seek slider, and live MAVLink Inspector.

The Charting tab lets you select any logged field for time-series plotting with zoom and a value popup showing current, minimum, and maximum values.
The Map tab shows the GPS flight path auto-fitted to the screen with an altitude mini-chart below it; clicking or dragging on the altitude chart moves a position marker on the map.
The Parameters tab lists all recorded parameters with a "changed only" filter.

### Onboard Logs — FTP Download Tab (Analyze View)

A new **Onboard Logs (FTP)** tab in Analyze View downloads logs from the vehicle using the MAVLink FTP protocol.
This provides a dedicated, explicit FTP path alongside the existing legacy log download method, making it easier to retrieve logs from vehicles that expose them via FTP.

### ArduPilot Logging Configuration (Vehicle Config)

A new **Logging** page in Vehicle Config lets you configure ArduPilot logging parameters directly from QGroundControl, including storage backends (`LOG_BACKEND_TYPE`), data group bitmasks (`LOG_BITMASK`), rate limits, and disarmed logging options.
This replaces the need to manually edit these parameters in the parameter editor.

### Searchable Settings and Vehicle Config

Both the Application Settings pages and the Vehicle Config sidebar now have a **keyword search** field.
Typing a word filters the visible sections across all pages, making it much faster to find a specific setting without knowing which page it lives on.

### Settings Pages — JSON-Driven UI

All Application Settings pages are now generated from JSON metadata rather than hand-coded QML.
Settings are organized into collapsible sections with consistent layout.
A search bar at the top of each settings page filters visible settings in real time.

### Plan View Toolbar

The Plan View toolbar has been completely reworked:

- Explicit **Open**, **Save**, **Upload**, and **Clear** buttons replace the old File tool button.
- Vehicle upload and storage status are shown directly in the toolbar.
- **Save** and **Upload** buttons are highlighted when there are unsaved or un-uploaded changes.
- A hamburger menu (☰) provides additional options such as Save as KML and Download from vehicle.
- Plan creation from a template has moved to the right panel and is shown only when the plan is empty.

### Plan View — Mission Status Panel

A new collapsible **Mission Status** panel at the bottom of the Plan View shows terrain profile and mission statistics (distance, time, maximum altitude).
Switchable sub-panels let you toggle between terrain data and mission stats.

### Plan View — Click-to-Set Home Position

The home position in Plan View is now set by an explicit first click on the map rather than tracking the map center.
Mission editing tools (Takeoff, Waypoint, Pattern, Land, ROI) are disabled until the home position is placed, preventing accidentally placing items before the reference point is defined.

### Plan View — Plan Templates with Vehicle Class Filtering

The Plan Creator now filters available plan templates by vehicle class (multirotor, fixed wing, VTOL, rover, sub), showing only templates compatible with the connected or configured vehicle.

### ArduPilot — Servo Outputs Page (Vehicle Config)

A new **Servo Outputs** page in Vehicle Config provides real-time visualization and configuration of servo outputs for ArduPilot vehicles:

- Live PWM output bars updated in real time.
- Per-channel function assignment, direction reversal, and min/max/trim PWM editing.

### ArduPilot — Scripting Component (Vehicle Config)

A new **Scripting** page in Vehicle Config lets you manage Lua scripts on ArduPilot vehicles directly from QGroundControl:

- Browse, upload, and delete scripts stored on the vehicle via MAVLink FTP.
- No need for a separate file manager or SD card access.

### ArduPilot — Safety Failsafe UI Modernization

The ArduPilot Safety setup page has been modernized with vehicle-specific failsafe sections for Copter, Plane, and Rover.
Controls use consistent slider and radio-button patterns, and a new `FactBitMaskCheckBoxSlider` control makes bitmask parameters much easier to configure.

### Camera UI Rework

The camera management code and UI have been significantly reworked.
The camera UI now correctly respects camera capability flags and modes, and behavior is more consistent across different camera types and configurations.

### Joystick — Manual Control Extensions

Full support for MAVLink manual control extensions has been added to the Joystick configuration:

- Improved settings read/write with validation.
- Improved logging.

Note: joystick settings from earlier versions are not migrated; joysticks will need to be re-calibrated and re-enabled after updating.

### NTRIP RTK Correction Link

QGroundControl now includes a built-in NTRIP client for streaming RTK correction data to the vehicle.
Configure the NTRIP server, mount point, and credentials in Application Settings; QGroundControl connects and forwards corrections automatically.

### MAVLink 2 Message Signing

The [Telemetry settings](settings_view/telemetry.md) page now includes a **MAVLink 2 Signing** section.
Signing cryptographically authenticates all messages between QGroundControl and the vehicle, preventing unauthorized command injection.
Keys are created by entering a friendly name and passphrase; only the SHA-256 hash is stored.

### GPS / RTK GPS Toolbar Indicator Improvements

The GPS/RTK GPS toolbar indicator now shows RTK correction link status even when no vehicle is connected, making it easier to verify RTK base station reception before flight.
A separate **GPS Resilience** indicator appears when the vehicle reports GPS spoofing or jamming state, with per-GPS details in the dropdown.

### MAVLink v1 Removed

Support for the legacy MAVLink v1 message protocol has been removed.
QGroundControl now requires MAVLink v2.

### Qt Framework Update

The Qt framework has been updated from 6.8.3 to 6.10.1, and the charting library has been migrated from the deprecated Qt Charts to the new Qt Graphs module.
