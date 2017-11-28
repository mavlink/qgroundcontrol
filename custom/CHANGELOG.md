## DataPilot Release Changelog

### v1.2.10 - Nov 28 2017

*   Dealing with set wifi password corner cases

### v1.2.9 - Nov 26 2017

*   Display animation while camera is busy (instead of black screen)
*   Disable time lapse when capturing images while in video mode
*   Add CGOET to list of cameras for survey

### v1.2.8 - Nov 16 2017

*   Restore Structure Scan

### v1.2.7 - Nov 14 2017

*   Enable camera settings even if no microSD card is present

### v1.2.6 - Nov 10 2017

*   Add hobbs meter

### v1.2.5 - Nov 8 2017

*   Disable structure scan (incomplete)

### v1.2.4 - Nov 7 2017

*   Show "Format Complete" message

### v1.2.3 - Nov 7 2017

*   No code changes: 
    Deployment of all branches to s3; the branch needs to be enabled on Jenkins

### v1.2.2 - Nov 2 2017

*   Internal localization work. Not visible to the outside yet.

### v1.2.1 - Nov 2 2017

*   Local thermal image capture (ST16 - CGOET)

### v1.2.0 - Nov 2 2017

*   No code changes. First 1.2.0 iteration and branching.

### v1.1.63 - Nov 2 2017

*   Upstream merge only. Last 1.1.x version.

### v1.1.62 - Nov 1 2017

*   Fix text edit field (camera parameters)

### v1.1.61 - Oct 31 2017

*   Thermal image PIP Mode not overlapping indicators. (CGOET)
*   Thermal image mode made persistent. (CGOET)

### v1.1.60 - Oct 31 2017

*   Updating presets (CGOET)

### v1.1.59 - Oct 31 2017

*   Fix first run dialog corner cases
*   Describe new password requirements

### v1.1.58 - Oct 30 2017

*   First time settings.

### v1.1.57 - Oct 27 2017

*   CGOET Presets (First Pass) Needs updated firmware for it to work. v1.0.07 is broken.

### v1.1.56 - Oct 26 2017

*   Don't play camera sound effect during mission.

### v1.1.55 - Oct 25 2017

*   Fixing thermal ROI persitency.

### v1.1.54 - Oct 25 2017

*   Adding flight data and battery status to UTM log.

### v1.1.53 - Oct 24 2017

*   Support for updater app

### v1.1.52 - Oct 23 2017

*   Fix time lapse slider initialization (it was resetting when created)
*   Skip initial double camera sound when doing time lapse

### v1.1.51 - Oct 23 2017

*   Ignore bad temperature readings (0C when taking pictures) (CGOET)

### v1.1.50 - Oct 23 2017

*   Fixed stopping time lapse using physical shutter button
*   Handle minimum interval possible depending on camera used

### v1.1.49 - Oct 23 2017

*   Time Lapse Photo

### v1.1.48 - Oct 22 2017

*   Export UTM logs

### v1.1.46 - Oct 17 2017

*   Temperature Range (CGOET). Note that it needs a new CGOET firmware for it to work.

### v1.1.45 - Oct 16 2017

*   Implemented Area/Spot ROI (CGOET)
*   Adjusted graphics showing "Area" size (CGOET)
*   Added a slider to control the thermal image opacity (when in Blend Mode)
*   Turn off live video when thermal image mode is set to Full

### v1.1.44 - Oct 13 2017

*   Prevent change settings while recording video

### v1.1.43 - Oct 13 2017

*   Major updates to CGOET handling

### v1.1.42 - Oct 12 2017

*   Added proper palette bars to UI.

### v1.1.41 - Oct 11 2017

*   When switching from the ET to some other camera, DataPilot was not "forgetting" the thermal image.

### v1.1.40 - Oct 11 2017

*   Merge upstream changes and fixes

### v1.1.39 - Oct 02 2017

*   Temperature readings from "Custom" area instead of "Full Area". This is until we have a proper Custom/Full toggle function.
*   Fixed issue with editing camera parameters (ET Only)
*   Upstream merge (DataPilot now in sync with upstream QGC)

### v1.1.36 - Sep 28 2017

*   Changed edit fields in CGOET settings

### v1.1.35 - Sep 25 2017

*   Preliminary support for the CGOET

### v1.1.34 - Sep 15 2017

*   Disable settings when image capture is busy.

### v1.1.33 - Sep 13 2017

*   Link video stream timeout with MAVLink connection timeout.

### v1.1.32 - Sep 12 2017

*   Added OBS indicator to the toolbar (flashes green when active)

### v1.1.31 - Sep 12 2017

*   Increased video stream timeout from 2 to 10 seconds (holding last frame before giving up)
*   Keep requesting image capture status while camera reports busy

### v1.1.30 - Sep 12 2017

*   Fixed changelog deploy

### v1.1.25 - Sep 8 2017

*   Adding changelog

### v1.1.24 - Sep 8 2017

*   Enable logging by default

### v1.1.23 - Sep 7 2017

*   Disable hardware shutter button when recording video.

### v1.1.22 - Sep 7 2017

*   Disable settings that cannot be changed while video is being recorded

    * Video Resolution
    * Video Format (for the E90)
    * Reset Camera
    * Format SD Card

### v1.1.21 - Sep 4 2017

*   Add ability to toggle video stream in full screen (hinding all controls)

### v1.1.20 - Sep 2 2017

* Add a timeout for gimbal calibration.
* Restrict spot area within image region.
* Fix wrong logic that enables/disables hardware shutter buttons.
* Prevent RC calibration if bound to a vehicle (RC calibration cannot be done while connected (and bound) to a vehicle.)

### v1.1.19 - Sep 2 2017

* Fix bad initializer for mission export (wrong file imported count)
* Limit negative gimbal pitch

### v1.1.18 - Aug 31 2017

* Disable local video recording (ST16)


