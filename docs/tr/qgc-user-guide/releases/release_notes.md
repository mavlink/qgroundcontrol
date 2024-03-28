# Release Notes

This topic contains the cumulative release notes for _QGroundControl_.

:::info
Stable build major/minor numbers are listed below.
_Patch_ release numbers are not listed, but can be found on the [Github release page](https://github.com/mavlink/qgroundcontrol/releases).
:::

## Stable Version 4.0 (current)

:::info
The format for Settings in QGC had to change in this release. Which means all QGC settings will be reset to defaults.
:::

- Settings
  - Language: Allow selection of language
  - Optional [CSV Logging](../settings_view/csv.md) of telemetry data for improved accessibility.
  - ArduPilot
    - Mavlink: Configurable stream rate settings
- Setup
  - Joystick
    - New joystick setup ui
    - Ability to configure held button to single or repeated action
  - ArduPilot
    - Motor Test
    - ArduSub
      - Automatic motor direction detection
- Plan
  - Create Plan from template with wizard like progression for completing full Plan.
  - Survey: Save commonly used settings as a Preset
  - Polygon editing
    - New editing tools ui
    - Support for tracing a polygon from map locations
  - ArduPilot
    - Support for GeoFence and Rally Points using latest firmwares and mavlink v2
- Fly
  - Click to ROI support
  - Added support for connecting to ADSB SBS server. Adds support for ADSB data from USB SDR Dongle running 'dump1090 --net' for example.
  - Ability to turn on Heading to home, COG and Next Waypoint heading indicators in Compass.
  - Video
    - Add support for h.265 video streams
    - Automatically add a [Video Overlay](../fly_view/video_overlay.md) with flight data as a subtitle for locally-recorded videos
  - Vehicle type specific pre-flight checklists. Turn on from Settings.
- Analyze
  - New Mavlink Inspector which includes charting support. Supported on all builds including Android and iOS.
- General
  - Released Windows build are now 64 bit only
  - Log Replay: Ability to specify replay speed
  - ArduPilot
    - Improved support for chibios firmwares and ArduPilot bootloader with respect to flashing and auto-connect.

Additional notes for some features can be found here: [v4.0 (Additional Notes)](../releases/stable_v4.0_additional.md).

## Stable Version 3.5

This section contains a high level and _non-exhaustive_ list of new features added to _QGroundControl_ in version 3.5.

- **Overall**
  - Added Airmap integration to QGC. OSX build only.
  - Bumped settings version (now 8).
    This will cause all settings to be reset to defaults.
  - Added Chinese and Turkish localization and partial German localization.
  - Added support for the Taisync 2.4GHz ViUlinx digital HD wireless link.
  - Fix loading of parameters from multiple components.
    This especially affected WiFi connections.
  - **ArduPilot** Support for ChibiOS firmware connect and flash.
- **Settings**
  - **RTK** Add support for specifying fixed RTK based station location in Settings/General.
  - **GCS Location**
    - Added UDP Port option for NMEA GPS Device.
    - GCS heading shown if available
- **Plan**
  - **Polygons** Support loading polygons from SHP files.
  - **Fixed Wing Landing Pattern** Add stop photo/video support.
    Defaults to on such that doing an RTL will stop camera.
  - **Edit Position dialog** Available on polygon vertices.
- **Fly**
  - **Camera Page** Updated support for new MAVLInk camera messages.
    Camera select, camera mode, start/stop photo/video, storage mangement...
  - **Orbit** Support for changing rotation direction.
  - **Instrument Panel**
    - Added ESTIMATOR_STATUS values to new estimatorStatus Vehicle FactGroup.
      These are now available to display in instrument panel.
    - Make Distance to GCS available for display from instrument panel.
    - Make Heading to Home available for display from instrument panel.

## Stable Version 3.4

This section contains a high level and _non-exhaustive_ list of new features added to _QGroundControl_ in version 3.4. Not to mention the large number of bug fixes in each stable release.

- **Settings**
  - **Offline Maps**
    - Center Tool allows you to specify a map location in lat/lon or UTM coordinates. Making it easier to get to the location you want to create an offline map for.
    - Ability to pre-download terrain heights for offline use.
  - **Help** Provides links to QGC user guide and forums.

- **Setup**
  - **Firmware** Ability to flash either PX4 or ArduPilot Flow firmware.
  - PX4 Pro Firmware
    - **Flight Modes** Specify channels for all available transmitter switches.
    - **Tuning: Advanced** Initial implementation of vehicle PID tuning support. Note that this is a work in progress that will improve in 3.5 daily builds.
  - ArduPilot Firmware
    - **Power/Safety** Support for new multi-battery setup.
    - **Trad Heli** New setup page.

- **Plan**

  - **File Load/Save** New model for Plan file loading which matches a standard File Load/Save/Save As user model.

  - **Load KML** Ability to load a KML file directly from the Sync menu. You will be prompted for what type of Pattern you want to create from the KML if needed.

  - **Survey** Better support for irregular shaped polygons.

  - **[Corridor Scan](../plan_view/pattern_corridor_scan.md)** - Create a flight pattern which follows a poly-line. For example can be used to survey a road.

  - **[Fixed Wing Landing Pattern](../plan_view/pattern_fixed_wing_landing.md)**
    - Landing area visually represented in Plan.
    - Landing position/heading can be copied from Vehicle position/heading.

  - **Terrain**

    - Height of mission items can be specified as height above terrain.
    - Survey and Corridor Scan can generate flight plans which follow terrain.

      ::: info
      This feature does not support [ArduPilot terrain following](http://ardupilot.org/copter/docs/common-terrain-following.html).
      :::

  - **Edit Position** Set item position from vehicle position.

- **Fly**
  - **Pre-Flight Checklist** You can turn this on from Settings. It provides a generic checklist to follow prior to flight. Expect more feature to appear for this in 3.5 daily builds.
  - **Instrument Panel**
    - Many new values available for display.
    - New Camera page which provides full camera control. Requires a camera which support new MavLink camera specification.
  - **ArduPlane** Much better support for guided commands including QuadPlane support.
  - **High Latency Links** Support for high latency links such as satellite connections.
    Limits the traffic from QGC up to Vehicle on these links to reduce cost.
    Supports HIGH_LATENCY MavLink message.
    Supports failover back/forth from high latency to normal link with dual link setup.

## Stable Version 3.3

:::tip
More detailed release notes for version 3.3 can be found [here](../releases/stable_v3.3_long.md).
:::

This section contains a high level and _non-exhaustive_ list of new features added to _QGroundControl_ in version 3.3. Not to mention the large number of bug fixes of this release.

- **Settings**
  - Local NMEA GPS device support.
  - Video Recording save settings.
- **Setup**
  - **Parameter Editor** - Searching updates as you type characters for near immediate response to searches.
  - **Joystick** - Android joystick support.
- **Plan**
  - **NEW - Structure Scan Pattern** - Create a multi-layered flight pattern that captures images over vertical surfaces (polygonal or circular). Used for 3d model generation or vertical surface inspection.
  - **Fixed Wing Landing Pattern** - You can now adjust the distance from the loiter to land point by either distance or glide slope fall rate.
  - PX4 GeoFence and Rally Point support.
  - Terrain height display in lower Mission Item altitude display
- **Fly**
  - Start/Stop video recording.
  - Better display of vehicle icons when connected to multiple vehicles.
  - Multi-Vehicle View supports commands which apply to all vehicles.
  - Displays vehicles reported from ADS-B sensor.
- **Analyze**
  - **Mavlink console** - New support for communicating with Mavlink console.
  - **Log Download** - Moved from Menu to Analyze view.

## Stable Version 3.2

:::tip
More detailed release notes for version 3.2 can be found [here](../releases/stable_v3.2_long.md).
:::

This section contains a high level and _non-exhaustive_ list of new features added to _QGroundControl_ in version 3.2.

- **Settings**

  - **File Save path** - Specify a save path for all files used by QGC.
  - **Telemetry log auto-save** - Telemetry logs are now automatically saved without prompting.
  - **AutoLoad Plans** - Used to automatically load a Plan onto a vehicle when it first connects.
  - **RTK GPS** - Specify the Survey in accuracy and Minimum observation duration.

- **Setup**

  - ArduPilot only
    - **Pre-Flight Barometer and Airspeed calibration** - Now supported
    - **Copy RC Trims** - Now supported

- **Plan View**

  - **Plan files** - Missions are now saved as .plan files which include the mission, geo-fence and rally points.
  - **Plan Toolbar** - New toolbar which shows you mission statistics and Upload button.
  - **Mission Start** - Allows you to specify values such as flight speed and camera settings to start the mission with.
  - **New Waypoint features** - Adjust heading and flight speed for each waypoint as well as camera settings.
  - **Visual Gimbal direction** - Gimbal direction is shown on waypoint indicators.
  - **Pattern tool** - Allows you to add complex patterns to a mission.
    - Fixed Wing Landing (new)
    - Survey (many new features)
  - **Fixed Wing Landing Pattern** - Adds a landing pattern for fixed wings to your mission.
  - **Survey** - New features
    - **Take Images in Turnarounds** - Specify whether to take images through entire survey or just within each transect segment.
    - **Hover and Capture** - Stop vehicle at each image location and take photo.
    - **Refly at 90 degree offset** - Add additional pattern at 90 degree offset to original so get better image coverage.
    - **Entry location** - Specify entry point for survey.
    - **Polygon editing** - Simple on screen mechanism to drag, resize, add/remove points. Much better touch support.

- **Fly View**

  - **Arm/Disarm** - Available from toolbar.
  - **Guided Actions** - New action toolbar on the left. Supports:
    - Takeoff
    - Land
    - RTL
    - Pause
    - Start Mission
    - Resume Mission - after battery change
    - Change Altitude
    - Land Abort
    - Set Waypoint
    - Goto Location
  - **Remove mission after vehicle lands** - Prompt to remove mission from vehicle after landing.
  - **Flight Time** - Flight time is shown in instrument panel.
  - **Multi-Vehicle View** - Better control of multiple vehicles.

- **Analyze View** - New

  - **Log Download** - Moved to Analyze view from menu
  - **Mavlink Console** - NSH shell access

- **Support for third-party customized QGroundControl**
  - Standard QGC supports multiple firmware types and multiple vehicle types. There is now support in QGC which allows a third-party to create their own custom version of QGC which is targeted specifically to their custom vehicle. They can then release their own version of QGC with their vehicle.

## Stable Version 3.1

New Features

- [Survey](../plan_view/pattern_survey.md) mission support
- [GeoFence](../plan_view/plan_geofence.md) support in Plan View
- [Rally Point](../plan_view/plan_rally_points.md) support in Plan View (ArduPilot only)
- ArduPilot onboard compass calibration
- Parameter editor search will now search as you type for quicker access
- Parameter display now supports unit conversion
- GeoTag images from log files (PX4 only)
- System health in instrument panel
- MAVLink 2.0 support (no signing yet)

Major Bug Fixes

- Fixed crash after disconnect from Vehicle
- Fixed android crash when using SiK Radios
- Many multi-vehicle fixes
- Bluetooth fixes
