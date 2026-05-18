# What's New

This page highlights user-facing changes since the last stable release (V5.0).

## Fly View

### Toolbar Improvements

- Individual toolbar dropdowns can now be expanded to show app/vehicle settings useful during flight, reducing the need to navigate to Vehicle Configuration or Application Settings between flights.
- **Flight Modes**: Configurable list allows you to remove unused flight modes from the toolbar.
- **Battery**: Dynamic bars with configurable thresholds (100%, Config 1, Config 2, Low, Critical).
- The **GPS/RTK GPS** indicator now shows RTK correction link status even when no vehicle is connected, making it easier to verify RTK base station reception before flight. A separate **GPS Resilience** indicator appears when the vehicle reports GPS spoofing or jamming state, with per-GPS details in the dropdown.

### Instrument Selection

Right-click on desktop or long-press on mobile to switch between different instrument clusters.

### Camera UI Rework

The camera management code and UI have been significantly reworked.
The camera UI now correctly respects camera capability flags and modes, and behavior is more consistent across different camera types and configurations.

### Guided Mode and Multi-Vehicle

- Better support for Guided GoTo Location, Orbit, and Fixed Wing Loiter.
- Multi-vehicle support improvements: configurable telemetry display and better support for applying actions to all vehicles.

## Plan View

### Toolbar Rework

The Plan View toolbar has been completely reworked:

- Explicit **Open**, **Save**, **Upload**, and **Clear** buttons replace the old File tool button.
- Vehicle upload and storage status are shown directly in the toolbar.
- **Save** and **Upload** buttons are highlighted when there are unsaved or un-uploaded changes.
- A hamburger menu (☰) provides additional options such as Save as KML and Download from vehicle.
- Plan creation from a template has moved to the right panel and is shown only when the plan is empty.

### Mission Status Panel

A new collapsible **Mission Status** panel at the bottom of the Plan View shows terrain profile and mission statistics (distance, time, maximum altitude).
Switchable sub-panels let you toggle between terrain data and mission stats.

### Click-to-Set Home Position

The home position in Plan View is now set by an explicit first click on the map rather than tracking the map center.
Mission editing tools (Takeoff, Waypoint, Pattern, Land, ROI) are disabled until the home position is placed, preventing accidentally placing items before the reference point is defined.

### Plan Templates with Vehicle Class Filtering

The Plan Creator now filters available plan templates by vehicle class (multirotor, fixed wing, VTOL, rover, sub), showing only templates compatible with the connected or configured vehicle.

### Multiple Fixed Wing Landing Sequences

Multiple fixed wing landing sequences can now be planned for landings at different locations.

## Vehicle Configuration

Vehicle Configuration has been renamed from Vehicle Setup and is now intended mainly for initial vehicle design and configuration, not changes between flights.

### ArduPilot — Servo Outputs

A new **Servo Outputs** page provides real-time visualization and configuration of servo outputs for ArduPilot vehicles:

- Live PWM output bars updated in real time.
- Per-channel function assignment, direction reversal, and min/max/trim PWM editing.

### ArduPilot — Scripting

A new **Scripting** page lets you manage Lua scripts on ArduPilot vehicles directly from QGroundControl:

- Browse, upload, and delete scripts stored on the vehicle via MAVLink FTP.
- No need for a separate file manager or SD card access.

### ArduPilot — Safety Failsafe Modernization

The ArduPilot Safety setup page has been modernized with vehicle-specific failsafe sections for Copter, Plane, and Rover.
Controls use consistent slider and radio-button patterns, and a new `FactBitMaskCheckBoxSlider` control makes bitmask parameters much easier to configure.

### ArduPilot — Logging Configuration

A new **Logging** page lets you configure ArduPilot logging parameters directly from QGroundControl, including storage backends (`LOG_BACKEND_TYPE`), data group bitmasks (`LOG_BITMASK`), rate limits, and disarmed logging options.

### Joystick — Manual Control Extensions

Full support for MAVLink manual control extensions has been added to the Joystick configuration:

- Improved settings read/write with validation.
- Improved logging.

::: warning
Joystick settings from earlier versions are not migrated; joysticks will need to be re-calibrated and re-enabled after updating.
:::

## Application Settings

### Searchable Settings and Vehicle Config

Both the Application Settings pages and the Vehicle Config sidebar now have a **keyword search** field.
Typing a word filters the visible sections across all pages, making it much faster to find a specific setting without knowing which page it lives on.

### JSON-Driven Settings UI

All Application Settings pages are now generated from JSON metadata rather than hand-coded QML.
Settings are organized into collapsible sections with consistent layout.

### MAVLink Actions

New and updated MAVLink Actions support in Fly View settings.

### Virtual Joystick

Virtual Joystick now supports left-handed mode.

### NTRIP RTK Correction Link

QGroundControl now includes a built-in NTRIP client for streaming RTK correction data to the vehicle.
Configure the NTRIP server, mount point, and credentials in Application Settings; QGroundControl connects and forwards corrections automatically.

### MAVLink 2 Message Signing

The Telemetry settings page now includes a **MAVLink 2 Signing** section.
Signing cryptographically authenticates all messages between QGroundControl and the vehicle, preventing unauthorized command injection.
Keys are created by entering a friendly name and passphrase; only the SHA-256 hash is stored.

### Configurable Stream Rates (ArduPilot)

Configurable telemetry stream rate support for ArduPilot vehicles.

## Analysis & Logs

### Log Viewer

The Log Viewer (**Analyze > Log Viewer**) is a new unified post-flight analysis tool that supports three log formats:

- **ArduPilot DataFlash** (`.bin` / `.log`) — field charting, GPS flight path map, parameter inspection, and status messages.
- **PX4 ULog** (`.ulg`) — same capabilities using PX4 log format.
- **MAVLink Telemetry** (`.tlog`) — playback with speed control, seek slider, and live MAVLink Inspector.

The Charting tab lets you select any logged field for time-series plotting with zoom and a value popup showing current, minimum, and maximum values.
The Map tab shows the GPS flight path auto-fitted to the screen with an altitude mini-chart below it; clicking or dragging on the altitude chart moves a position marker on the map.
The Parameters tab lists all recorded parameters with a "changed only" filter.

### Onboard Logs — FTP Download

A new **Onboard Logs (FTP)** tab in Analyze View downloads logs from the vehicle using the MAVLink FTP protocol, providing a dedicated FTP path alongside the existing legacy log download method.

## General

### Touch Screen UI Updates

Focused UI updates for smaller touch screens used by integrated controllers such as Herelink:

- New slider controls for value entry.
- Better touch support for various controls.

### MAVLink v1 Removed

Support for the legacy MAVLink v1 message protocol has been removed.
QGroundControl now requires MAVLink v2.

### Qt Framework Update

The Qt framework has been updated from 6.8.3 to 6.10.1, and the charting library has been migrated from the deprecated Qt Charts to the new Qt Graphs module.

### Build System

The build system has been fully converted to CMake (qmake is no longer supported).
GStreamer support updated to 1.22.
