## DataPilot Release Changelog

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


