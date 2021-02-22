# QGroundControl Change Log

Note: This file only contains high level features or important fixes.

## 4.1 - Daily build

### 4.1.2 - Not yet released
* Bug: Radio setup - Fix double send of `MAV_CMD_PREFLIGHT_CALIBRATION` causing "Unable to send command" error.

### 4.1.1 - Stable
* Fix TCP link comms

### 4.1.0

* Support simple cameras which only support DIGICAM_CONTROL in the Photo/Video control on Fly View.
* Load Parameters From File: Support loading parameters which don't currently existing on the vehicle.
* Load Parameters From File: Add dialog which shows diff of file and vehicle params. Selective param upload from file.
* Video Streaming: New camera control supports capturing individual images from the stream
* Fly: Press and hold on arm button will change it to Force Arm. Click again to force arm.
* VTOL: General setting for transition distance which affects Plan takeoff, landing pattern creation
* VTOL: Much better VTOL support throughout QGC
* Maps: Support zoom up to level 23 even if map provider doesn't provide tiles that high
* Settings/Mavlink: Add ability to forward mavlink traffic out specified UDP port
* Support mavlink terrain protocol which queries gcs for terrain height information. Allows planning missions with TERRAIN\_FRAME.
* Fly: New instrument values display/editing support
* Plan: Added new VTOL Landing Pattern support
* Plan: Much better conversion of missions to KML for 3d visualization/verification of missions
* Plan: New Terrain Profile display including terrain collision indications on profile and in patterns (Survey, CorridorScan, etc)
* Fly: Rearchitect view and controls within for much better customization support in custom builds

## 4.0

## 4.0.9 - Not yet released

* Don't auto-connect to second Cube Orange/Yellow composite port
* Plan: Fix bugs associated with mission commands which specify and altitude but no lat/lon
* Fix bug which could prevent view switching from working after altitude mode warning dialog would pop up

## 4.0.8 - Stable

* iOS: Modify QGC file storage location to support new Files app
* Mobile: Fix Log Replay status bar file selection

## 4.0.7 - Stable

* Fix video page sizing
* Virtual Joystick: Fix right stick centering. Fix/add support for rover/sub reverse throttle support.
* Fix display of multiple ADSB vehicles

### 4.0.6 - Stable

* Analyze/Log Download - Fix download on mobile versions of QGC
* Fly: Fix problems where Continue Mission and Change Altitude were not available after a Mission Pause.
* PX4 Flow: Fix video display problem

### 4.0.5 - Stable

* Solo: Fix mission upload failures
* Plan: Fix crash when using Create Plan - Survey for fixed wing vehicle

### 4.0.4

* Mobile File Save: Fix problem with incorrect file extension being added
* Radio Setup: Fix problem with Spektrum bind
* Plan/Fly: Bring back waypoint number display in map items

### 4.0.3

* Plan: Add setting for takeoff item not required
* Plan: Takeoff item must be added prior to allowing other item types to enable
* Video: Add low latency mode as optional configuration setting (defaults to false)
* ArduPilot: Fix generated list of available firmwares

### 4.0.2

* Fix Mavlink V2 protocol negotation based on capability bits
* Fix waiting for AUTOPILOT_VERSION response to get capability bits
* ArduPilot: Above two fixes make fence/rally support enabling more reliable

### 4.0.1

* Fix ArduPilot current mission item tracking in Fly view
* Fix ADSB vehicle display
* Fix map positioning bug in Plan view
* Fix Windows 0xcc000007b startup error causes by incorrect VC runtimes being installed.

### 4.0.0

* Added ROI option during manual flight.
* Windows: Move builds to 64 bit, Qt 5.12.5
* Plan: ROI button will switch to Cancel ROI at appropriate times
* Plan: When ROI is selected the flight path lines which are affected by the ROI will change color
* ADSB: Added support for connecting to SBS server. Adds support for ADSB data from USB SDR Dongle running 'dump1090 --net' for example.
* Toolbar: Scrollable left/right on small screens like phones
* Plan View: New create plan UI for initial plan creation
* New Corridor editing tools ui. Includes ability to trace polyline by clicking.
* New Polygon editing tools ui. Includes ability to trace polygon by clicking.
* More performant flight path display algorithm. Mobile builds no longer show limited path length.
* ArduPilot: Add Motor Test vehicle setup page
* Compass Instrument: Add indicators for Home, COG and Next Waypoint headings.
* Log Replay: Support changing speed of playback
* Basic object avoidance added to vehicles.
* Added ability to set a joystick button to be single action or repeated action while the button is held down.
* Rework joysticks. Fixed several issues and updated setup UI.
* Adding support for UDP RTP h.265 video streams
* For text to speech engine on Linux to English (all messages are in English)
* Automated the ingestion of localization from Crowdin
* Automated the generation of language resources into the application
* Added all languages that come from Crowdin, even if empty.
* Allow dynamic language changes
* Check and respect camera storage status
* QGC now requires Qt 5.11 or greater. The idea is to standardize on Qt 5.12 (LTS). Just waiting for a solution for Windows as Qt dropped support for 32-bit.
* New, QtQuick MAVLink Inspector. The basics are already there but it still needs the ability to filter compID.
* Fixed application storage location on iOS. It was trying to save things where it could not.
* Basic support for secondary, thermal imaging with video streaming. If a camera provides both visual spectrum and thermal imaging, you have the option of displaying both at the same time.
* Better handling of fonts for Korean and Chinese locales. QGC now has builtin fonts for Korean (where some unusable font was being used). I still need to know if Chinese will need its own font as well.
* ArduPilot: Copter - Add suppor for Simple and Super Simple flight modes
* ArduPilot: Flight Mode setup - Switch Options were not showing up for all firmware revs
* ArduCopter: Add PID Tuning page to Tuning Setup
* ArduPilot: Copter - Advanced Tuning support
* ArduPilot: Rover - Frame setup support
* ArduPilot: Copter - Update support to 3.5+
* ArduPilot: Plane - Update support to 3.8+
* ArduPilot: Rover - Update support to 3.4+
* ArduPilot: Rework Airframe setup ui
* Plan/Pattern: Support named presets to simplify commonly used settings setup. Currently only supported by Survey.
* ArduCopter: Handle 3.7 parameter name change from CH#_OPT to RC#_OPTION.
* Improved support for flashing/connecting to ChibiOS bootloaders boards.
* Making the camera API available to all firmwares, not just PX4.
* ArduPilot: Support configurable mavlink stream rates. Available from Settings/Mavlink page.
* Major rewrite and bug fix pass through Structure Scan. Previous version had such bad problems that it can no longer be supported. Plans with Structure Scan will need to be recreated. New QGC will not load old Structure Scan plans.

## 3.5

### 3.5.5
* Fix mavlink message memset which cause wrong commands to be sent on ArduPilot GotoLocation.
* Disable Pause when fixed wing is on landing approach.

### 3.5.4
* Update windows drivers
* Add support for FMUK66 flashing/connection
* Guard against null geometry coming from gstreamer which can cause crashes
* Add .apj file selection support to custom firmware flash

### 3.5.3
* Change minimum RTK Survey-In limit to 0.01 meters
* Change Windows driver detection logic
* Fix crash when clicking on GeoFence polygon vertex
* PX4: Fix missing ```MC_YAW_FF``` parameter in PID Tuning
* ArduPilot: Fix parameter file save generating bad characters from git hash

### 3.5.2
* Fix Ubuntu AppImage startup failure

### 3.5.1
* Update Windows usb drivers
* Add ArduPilot CubeBlack Service Bulletin check
* Fix visibility of PX4/ArduPilot logo in toolbar
* Fix tile set count but in OfflineMaps which would cause image and elevation tile set to have incorrect counts and be incorrectly marked as download incomplete.

### 3.5.0
* Plan GeoFence: Fix loading of fence from intermediate 3.4 code
* Structure Scan: Fix loading of structure scan height
* ArduPilot: Fix location of planned home position when not connected to vehicle. Issue #6840.
* Fix loading of parameters from multiple components. Would report download complete too early, thus missing all default component params.
* Fix file delete in mobile file dialogs
* Add support for specifying fixed RTK based station location in Settings/General.
* Added Airmap integration to QGC
* Added ESTIMATOR_STATUS values to new estimatorStatus Vehicle FactGroup. These are now available to display in instrument panel.
* Added Chinese and Turkish localization and partial German localization. 
* Make Distance to GCS available for display from instrument panel.
* Make Heading to Home available for display from instrument panel.
* Edit Position dialog available on polygon vertices.
* Fixed Wing Landing Pattern: Add stop photo/video support. Defaults to on such that doing an RTL will stop camera.
* Support loading polygons from SHP files
* Bumped settings version (now 8). This will cause all settings to be reset to defaults.
* Orbit visuals support changing rotation direction
* Added support for the Taisync 2.4GHz ViUlinx digital HD wireless link.
* Added UDP Port option for NMEA GPS Device.

## 3.4

### 3.4.4
* Stable desktop versions now inform user at boot if newer version is available.
* Multi-Vehicle Start Mission and Pause now work correctly. Issue #6864.

### 3.4.3
* Fix bug where Resume Mission would not display correctly in some cases. Issue #6835.
* Fix Planned Home Position altitude when no terrain data available. Issue #6846.

### 3.4.2
* Fix bug where new mission items may end up with 0 altitude internally and sent to vehicle while UI shows correct altitude. Issue #6823.

### 3.4.1
* Fix crash when Survery with terrain follow is moved quickly
* Fix terrain follow climb/descent rate fields swapped in ui

