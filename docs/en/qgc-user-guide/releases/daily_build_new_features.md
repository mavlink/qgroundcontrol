# Daily Build Major Changes

This topic contains a high level and _non-exhaustive_ list of new features added to _QGroundControl_ since the last [stable release](../releases/release_notes.md).
These features are available in [daily builds](../releases/daily_builds.md).
There is also a [Change Log](https://github.com/mavlink/qgroundcontrol/blob/master/ChangeLog.md) available for viewing.

* Fly View
  * Toolbar
    * Individual dropdowns can now be expanded
      * The expanded pages contain app/vehicle settings which are useful after initial vehicle configuration
      * Goal is not have to dive back into more complex Vehicle Configuration or Application Settings pages from flight to flight
    * Flight Modes: Configurable list allows you to remove unused flight modes
    * Battery: Dynamic bars with configurable thresholds (100%, Config 1, Config 2, Low, Critical)
  * Instrument Selection: Right-click on desktop or long‑press on mobile to switch between different instrument clusters
  * 3D View allows you to load an OSM file as the 3D map
  * Better support for Guided GoTo Location, Orbit, Fixed Wing Loiter
  * Multi-vehicle support
    * Configurable telemetry display
    * Better support for applying actions to all vehicles
* Plan View
  * Multiple fixed wing landing sequences can now be planned for landings at different locations
* Vehicle Configuration
  * Renamed from Vehicle Setup
  * Now should be mainly used for initial vehicle design/config, not changes flight to flight
* Application Settings
  * Application settings categories have been restructed/redesigned for easier access
  * Fly View
    * New/Updated MAVLink Actions support
    * Virtual Joystick support for left handed moded
  * Telemetry
    * Added support for MAVLink 2 signing
    * Configurable stream rate support for ArduPilot vehicles
* Overall
  * Focused UI updates for smaller touch screens
    * Used by integrated controllers such as a Herelink
    * New sliders controls for value entry
    * Better touch support for various controls
  * Many, many bugs fixed
* Developer changes
  * Build system fully converted to cmake
    * qmake no longer supported
  * Source updated to use Qt 6.8.3
  * GStreamer support updated to 1.22