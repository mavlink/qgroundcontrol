# What's New

This page highlights user-facing changes since the last stable release (V5.1).

## Fly View

### Toolbar Indicator Improvements

- **Flight Modes** — mode changes now require a hold to confirm, and VTOL transition is handled from the indicator.
- **GPS Resilience** — new indicator appears when the vehicle reports GPS spoofing or jamming state, with per-GPS details in the dropdown.
- **ESC Status** — new indicator showing ESC telemetry.
- **Gimbal** — reworked to match the new indicator UI style.
- **MAVLink Signing** — new indicator shows whether MAVLink 2 message signing is active on the current vehicle connection, with named key management in Application Settings and per-vehicle key auto-detection.
- **Remote ID** — live status (status flags, GCS position) now lives solely in the indicator's expanded page, while the Remote ID page in Application Settings contains only settings.

### Camera UI Rework

The camera management code and UI have been significantly reworked.
The camera UI now correctly respects camera capability flags and modes, and behavior is more consistent across different camera types and configurations.

### Vehicle Actions

Better support for Go To Location, Orbit, and Fixed Wing Loiter actions.

### Multi-Vehicle

Multi-vehicle support improvements: configurable telemetry display and better support for applying actions to all vehicles.

### 3D View Camera Controls

The 3D View free-fly camera has been replaced with Gazebo-style cursor-anchored controls — the ground point you grab stays under the cursor while you pan, orbit, and zoom.
Full mouse and touch gesture support is included, along with a scale bar showing ground distance.

## Plan View

### Unified Plan Tree

The right panel is now a single unified tree view of the entire plan, replacing the separate mission item list and GeoFence/Rally editors:

- **Plan Info**, **Defaults**, **Mission**, **GeoFence**, and **Rally Points** are shown as collapsible sections in one tree.
- Expanding the Mission, GeoFence, or Rally section makes it the active map editing layer — the sections are exclusive, so it is always clear which layer map clicks affect.
- Mission items are edited inline in the tree, including mission transforms.
- The tree automatically expands and scrolls to newly added or selected items, and the whole panel can be collapsed to maximize map space.

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

## Analyze View

### Log Viewer

The Log Viewer is a new unified post-flight analysis tool that supports three log formats:

- **ArduPilot DataFlash** (`.bin` / `.log`) — field charting, GPS flight path map, parameter inspection, and status messages.
- **PX4 ULog** (`.ulg`) — same capabilities using PX4 log format.
- **MAVLink Telemetry** (`.tlog`) — playback with speed control, seek slider, and live MAVLink Inspector.

The Charting tab lets you select any logged field for time-series plotting:

- Multiple fields plotted at once with a color-coded legend and per-field min/max/current values.
- Click or drag on the chart to place a cursor with a popup showing exact values at that time.
- Flight mode and event markers can be overlaid on the chart.
- Zoom into any time range; the X axis can show elapsed time or local clock time.

The Map tab shows the GPS flight path auto-fitted to the screen with an altitude mini-chart below it; clicking or dragging on the altitude chart moves a position marker on the map.
The Parameters tab lists all recorded parameters with a "changed only" filter.

### Onboard Logs

Downloading logs from the vehicle now automatically uses the best available transport: MAVLink FTP when the vehicle supports it, falling back to the legacy log transfer messages otherwise.
FTP transfers also allow deleting individual logs from the vehicle.

### MAVLink Inspector

- Messages are now separated by instance (e.g. `sensor_id`), so multiple sensors of the same type can be inspected individually.
- Numeric array fields expand into individual elements that can be charted separately.
- `NAMED_VALUE_*` and `DEBUG*` messages are separated by name for easier debugging.
- Charting improvements with more efficient data handling.

## Vehicle Configuration

Vehicle Configuration has been renamed from Vehicle Setup and is now intended mainly for initial vehicle design and configuration, not changes between flights.
Settings you may want to adjust from one flight to the next are instead available from the expanded pages of the toolbar indicators in Fly View.

- The sidebar is now a tree: each page expands to show the sections within it, and clicking a section jumps directly to it on the page.
- A new **keyword search** field filters the tree to matching pages and sections, making it much faster to find a specific setting.

### Parameter Editor

#### Tabbed Views and Favorites

The Parameter Editor now provides **Full List**, **Modified**, and **Favorites** tabs, replacing the old "Show modified only" checkbox.
Mark frequently used parameters as favorites via the new Fav column; favorites persist across sessions.

#### Read-Only Parameters

Parameters flagged as read-only by the vehicle are now shown as such in the UI and cannot be edited by default.
Advanced-UI users can deliberately override the lock via a **Force edit read-only param** checkbox for diagnostic and recovery cases.

#### Mission Planner File Import

The parameter diff dialog can now load Mission Planner `.param` files (whitespace-separated 5-column format) in addition to the existing QGC `.params` format.
Select a `.param` file using the new **Mission Planner Files (*.param)** filter in the load dialog; differences from the vehicle's current values are shown as usual before any changes are applied.

### Joystick

::: warning
Joystick settings from earlier versions are not migrated; joysticks will need to be re-calibrated and re-enabled after updating.
:::

#### Manual Control Extensions

Full support for MAVLink manual control extensions has been added to the Joystick configuration:

- Improved settings read/write with validation.
- Improved logging.

#### Additional Stick Axes

Up to 8 additional joystick axes can now be sent to the vehicle via MAVLink `RC_CHANNELS_OVERRIDE` extension channels (19–26), with a full calibration and settings workflow.

#### Camera Focus Actions

Joystick buttons can now be bound to **Step Focus In/Out** and **Continuous Focus In/Out** camera actions (for cameras that report basic focus support).

### ArduPilot

#### Servo Outputs

A new **Servo Outputs** page provides real-time visualization and configuration of servo outputs:

- Live PWM output bars updated in real time.
- Per-channel function assignment, direction reversal, and min/max/trim PWM editing.

#### Scripting

A new **Scripting** page lets you manage Lua scripts on the vehicle directly from QGroundControl:

- Browse, upload, and delete scripts stored on the vehicle via MAVLink FTP.
- No need for a separate file manager or SD card access.

#### Safety Failsafe Modernization

The Safety setup page has been modernized with vehicle-specific failsafe sections for Copter, Plane, and Rover.
Controls use consistent slider and radio-button patterns, and a new bitmask checkbox control makes bitmask parameters much easier to configure.

#### ESC Configuration

ESC configuration and calibration have been extracted from the Power page into a new standalone **ESC** setup page, with support for both `MOT_*` (Copter/Rover/Sub) and `Q_M_*` (QuadPlane) parameter prefixes.
The Power page now uses tabs to switch between Battery 1 and Battery 2.

#### Airspeed

A new **Airspeed** setup page provides Basic/Advanced tabbed configuration of airspeed sensors:

- **Basic**: sensor type, use airspeed, ratio, auto-calibrate, primary sensor selection, and airspeed limits.
- **Advanced**: per-sensor offset, pitot tube order, analog pin, I2C bus, PSI range, health monitoring, and options bitmask.

#### Logging Configuration

A new **Logging** page lets you configure logging parameters directly from QGroundControl, including storage backends (`LOG_BACKEND_TYPE`), data group bitmasks (`LOG_BITMASK`), rate limits, and disarmed logging options.

#### Configurable Stream Rates

Configurable telemetry stream rate support for ArduPilot vehicles.

## Application Settings

- The sidebar is now a tree: each page expands to show the sections within it, and clicking a section jumps directly to it on the page.
- A new **keyword search** field filters the tree to matching pages and sections across all pages, making it much faster to find a specific setting without knowing which page it lives on.

### General

#### Audio Mute

Muting audio output no longer zeroes out the volume slider — mute is now a separate setting, so your preferred volume level is preserved when unmuting.

### Fly View

#### Virtual Joystick

Virtual Joystick now supports left-handed mode.

### Plan View

#### Multiple Fixed Wing Landing Sequences

A new **Allow configuring multiple landing sequences** setting enables planning fixed wing landing sequences at different locations, allowing Return-to-Launch contingency selection.

### Comm Links

#### Custom Serial Baud Rates

Serial link and NMEA GPS baud rate selectors now include a **Custom** option with a numeric text field for entering arbitrary baud rates.

### NTRIP/RTK

#### Built-In NTRIP Client

QGroundControl now includes a built-in NTRIP client for streaming RTK correction data to the vehicle.
Configure the NTRIP server, mount point, and credentials; QGroundControl connects and forwards corrections automatically.

### Telemetry

#### Connecting to a Flying Vehicle

A new **Skip initial download when flying** setting skips parameter and mission plan downloads when connecting to a vehicle that is already armed, preventing bandwidth saturation on constrained links.
Toolbar indicators show a compact view immediately, and Vehicle Configuration offers a manual **Download Parameters** button when needed.

## General

### Android Improvements

- On Android 11+, application data (logs, plans, settings) is now saved to the root of a removable SD card when one is present, using the "All files access" permission.
- A new POSIX serial backend (selectable via an app setting) adds support for built-in serial ports (`/dev/ttyS*`, `/dev/ttyHS*`) on integrated controllers, in addition to USB serial devices.

### MAVLink v1 Removed

Support for the legacy MAVLink v1 message protocol has been removed.
QGroundControl now requires MAVLink v2.

## Developer

### JSON-Driven Settings UI

Application Settings pages and many Vehicle Configuration pages are now generated from JSON metadata rather than hand-coded QML.
Settings are organized into collapsible sections with consistent layout.

### Settings Override Files

Application settings can now be overridden — changing defaults, forcing values, hiding settings from the UI — without a custom build.
See [Settings Override Files](../../qgc-dev-guide/file_formats/settings_override.md) for details.

### Qt Framework Update

The Qt framework has been updated from 6.8.3 to 6.10.1, and the charting library has been migrated from the deprecated Qt Charts to the new Qt Graphs module.

### Build System

The build system has been fully converted to CMake (qmake is no longer supported).
GStreamer support updated to 1.22.

### API Documentation

Doxygen-generated API documentation is now built and published automatically by a weekly CI pipeline, available at <https://api.qgroundcontrol.com/master/annotated.html>.
