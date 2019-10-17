# QGroundControl Change Log

Note: This file only contains high level features or important fixes.

## 3.6

### 3.6.0 - Daily Build

* Major rewrite and bug fix pass through Structure Scan. Previous version had such bad problems that it can no longer be supported. Plans with Structure Scan will need to be recreated. New QGC will not load old Structure Scan plans.

### 3.5.6 - Not yet released

### 3.5.5 - Stable
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

