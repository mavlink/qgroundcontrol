## AGS Changelog

### July 24 2019

* Updated joystick settings
* Fixed video restart issue
* Microhard improvements
* Updated localization strings
* Warn users the effects of a full paramater reset (UAVCAN)

### July 08 2019

* Implemented 2D axes gimbal control using virtual joystick
* Separate individual battery status from battery summary

### June 30 2019

* Make guided altitude slider zero-based when vehicle is on the ground (positive values only)

### June 27 2019

* Added support for h.265 streams when using RTP UDP

### June 24 2019

* Handled cases where menus would show up at random places on Android
* Moved PrFlight Checklist to custom
* Force text to speech engine on Linux to English (all messages are in English)

### June 14 2019

* Made the Preflight Checklist dialog modal

### June 11 2019

* Adding support for thermal "palette" options within the camera control.
* Show battery icon for battery drop down indicator. The main toolbar will also now show the amount of the lowest battery on main battery indicator.

### June 10 2019

* Upstream merge
* Log replay
* Parameter meta data cleanup
* Tweaking toolstrip layout
* Update PX4 Firmware metadata
* Update MAVLink
* Correct frame on Rally mission items
* Overlay on recorded videos by using subtitle files (Code only, not implemented)

### June 6 2019

* Show second battery status (if it exists)
* Show vehicle options when offline editing missions

### June 3 2019

* Add gimbal control

### May 22 2019

* Change default metric system used based on system locale: https://www.wrike.com/open.htm?id=354738389
* Fix issue where when you select "Thermal Full", it was not hiding the visual spectrum video.
*	Tweaked the FOV proportion between the Sony and the Boson cameras.

### May 16 2019

*   Bitrate, encoding and fps settings from Matej
*   Fix for java open accessory and joystick issues
*   Automated the ingestion of localization from Crowdin
*   Automated the generation of language resources into the application
*   Added all languages that come from Crowdin, even if empty.
*   Allow dynamic language changes

### May 15 2019

*   Preload Settings, Vehicle Setup and Analyze views
*   Make drawer menu buttons slightly larger to make it easier to "touch" when using mobile.
*   Added some of the Analyze panels to mobile builds

### May 14 2019

*   Added Initial camera quick settings control (zoom and EV control)

### May 8 2019

*   Added Google Breakpad to Android builds

### April 30 2019

*   Hide Airframe, Radio, Flight Modes, Power, Tuning and Camera in normal mode
*   Fix Speed display to respect the configured metric system

### April 15 2019

*   Fix blurry icon. Issue: https://www.wrike.com/open.htm?id=340642934

### March 17 2019

*   Merging upstream master. It brings in support for the Microhard modem and fixes for MAVLink console on mobile platforms.

### March 13 2019

*   Formatting elapsed time as hh:mm:ss instead of sss.s

### March 11 2019

*   Merge upstream master. That brings Taisync fixes and additions from Matej.

### March 6 2019

*   Merge upstream master. That brings in the ability to filter paramaters to show only those that changed.

### March 5 2019

*   Added support for nested customization
*   Replaced the ToolStrip control with our own (stil needs customization)
*   Created this changelog

### March 4 2019

*   Fixed font sizing on Windows

### March 1 2019

*   Added an "Advanced Mode", which enables advanced UI components.

### February 28 2019

*   Added the message indicator to the main tool bar. This is still from upstream and needs customization.
*   Made the main toolbar background off a solid color (instead of the gradient)
*   Enabled Airmap (macOS only for now)
*   Merged upstream master (QGC) into AGS

### February 26 2019

*   Enable Taisync on Android
*   Make "Indoor" palette as default for everything (it was for desktop only)


