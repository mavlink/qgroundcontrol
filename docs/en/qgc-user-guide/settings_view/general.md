# General Settings (Settings View)

The general settings (**SettingsView > General Settings**) are the main place for application-level configuration.
Settable values include: display units, autoconnection devices, video display and storage, brand image, and other miscellaneous settings.

::: info
Values are settable even if no vehicle is connected. Settings that require a vehicle restart are indicated in the UI.
:::

## Units

This section defines the display units used in the application.

The settings are:

- **Distance**: Meters | Feet
- **Area**: SquareMetres | SquareFeet | SquareKilometers | Hectares | Acres | SquareMiles
- **Speed**: Metres/second | Feet/second | Miles/hour | Kilometres/hour | Knots
- **Temperature**: Celsius | Fahrenheit

## Miscellaneous

This section defines a number of miscellaneous settings, related to (non exhaustively): font sizes, colour schemes, map providers, map types, telemetry logging, audio output, low battery announcement levels, default mission altitude, [virtual joysticks](../settings_view/virtual_joystick.md), mission autoloading, default application file load/save path etc.

The settings are:

- <a id="language"></a>**Language**: System (System Language) | Bulgarian, Chinese, ...

  Translations are generally built into the application and selected automatically based on the system language.

  Metadata downloaded from the vehicle (such as parameter descriptions) might have translations as well.
  These are downloaded from the internet upon vehicle connection. The translations are then cached locally.
  This means an internet connection during vehicle connection is required at least once.

- <a id="colour_scheme"></a>**Color Scheme**: Indoor (Dark) | Outdoor (Light)
- **Map Provider**: Google | Mapbox | Bing | Airmap | VWorld | Eniro | Statkart
- **Map Type**: Road | Hybrid | Satellite
- **Stream GCS Position**: Never | Always | When in Follow Me Flight Mode.
- **UI Scaling**: UI scale percentage (affects fonts, icons, button sizes, layout etc.)
- **Mute all audio output**: Turns off all audio output.
- **Check for Internet Connection**: Uncheck to allow maps to be used in China/places where map tile downloads are likely to fail (stops the map-tile engine continually rechecking for an Internet connection).
- <a id="autoload_missions"></a> **Autoload Missions**: If enabled, automatically upload a plan to the vehicle on connection.
  - The plan file must be named **AutoLoad#.plan**, where the `#` is replaced with the vehicle id.
  - The plan file must be located in the [Application Load/Save Path](#load_save_path).
- **Clear all settings on next start**: Resets all settings to the default (including this one) when _QGroundControl_ restarts.
- **Announce battery lower than**: Battery level at which _QGroundControl_ will start low battery announcements.
- <a id="load_save_path"></a>**Application Load/Save Path**: Default location for loading/saving application files, including: parameters, telemetry logs, and mission plans.

## Data Persistence {#data_persistence}

The settings are:

- **Disable all data persistence**: Check to prevent any data being saved or cached: logs, map tiles etc.
  This setting disables the [telemetry logs section](#telemetry_logs).

## Telemetry Logs from Vehicle {#telemetry_logs}

The settings are:

- <a id="autosave_log"></a>**Save log after each flight**: Telemetry logs (`.tlog`) automatically saved to the _Application Load/Save Path_ ([above](#load_save_path)) after flight.
- **Save logs even if vehicle was not armed**: Logs when a vehicle connects to _QGroundControl_.
  Stops logging when the last vehicle disconnects.
- [**CSV Logging**](csv.md): Log subset of telemetry data to a CSV file.

## Fly View {#fly_view}

The settings are:

- **Use Preflight Checklist**: Enable pre-flight checklist in Fly toolbar.
- **Enforce Preflight Checklist**: Checklist completion is a pre-condition for arming.
- **Keep Map Centered on Vehicle**: Forces map to center on the currently selected vehicle.
- **Show Telemetry Log Replay Status Bar**: Display status bar for [Replaying Flight Data](../fly_view/replay_flight_data.md).
- **Virtual Joystick**: Enable [virtual joysticks](../settings_view/virtual_joystick.md) (PX4 only)
- **Use Vertical Instrument Panel**: Align instrument panel vertically rather than horizontally (default).
- **Show additional heading indicators on Compass**: Adds additional indicators to the compass rose:
- _Blue arrow_: course over ground.
- _White house_: direction back to home.
- _Green line_: Direction to next waypoint.

- **Lock Compass Nose-Up**: Check to rotate the compass rose (default is to rotate the vehicle inside the compass indicateor).
- **Guided Minimum Altitude**: Minimum value for guided actions altitude slider.
- **Guided Maximum Altitude**: Maximum value for guided actions altitude slider.
- **Go To Location Max Distance**: The maximum distance that a Go To location can be set from the current vehicle location (in guided mode).

## Plan View {#plan_view}

The settings are:

- **Default Mission Altitude**: The default altitude used for the Mission Start Panel, and hence for the first waypoint.

## AutoConnect to the following devices {#auto_connect}

This section defines the set of devices to which _QGroundControl_ will auto-connect.

Settings include:

- **Pixhawk:** Autoconnect to Pixhawk-series device
- **SiK Radio:** Autoconnect to SiK (Telemetry) radio
- **PX4 Flow:** Autoconnect to PX4Flow device
- **LibrePilot:** Autoconnect to Libre Pilot autopilot
- **UDP:** Autoconnect to UDP

RTK base receivers are auto-connected from the [GNSS Receiver](gnss_receiver.md#gnss-receiver) settings.

### Ground Station Location {#nmea_gps}

_QGroundControl_ will automatically use an internal GPS to display its own location on the map with a purple `Q` icon (if the GPS provides a heading, this will be also indicated by the icon).
It may also use the GPS as a location source for _Follow Me Mode_ - currently supported on [PX4 Multicopters only](https://docs.px4.io/en/flight_modes/follow_me.html).

You can also connect an external GNSS receiver that outputs ASCII NMEA (this is normally the case)
over a serial port, a TCP server, or a UDP port. Configure it in the
[GNSS Receiver](gnss_receiver.md#gnss-receiver) settings with the _Position only_ role, or with the
_Passive RTCM/NMEA_ role when the receiver also sends RTCM corrections that should be forwarded to
vehicles. Enable **Connect on startup** to connect it whenever QGC starts.

::: tip
A higher quality external GPS system may be useful even if the ground station has internal GPS support.
:::

The [GCS Position](gnss_receiver.md#gcs-position) setting chooses between the GNSS receiver and internal
positioning, or picks the best available source automatically.

Connection status is separate from position quality. An occupied UDP port, inaccessible serial
device, or port reserved by another connection is reported even before a GPS fix is available.

::: tip
To troubleshoot serial GPS problems: disable the GNSS receiver's auto-connect, close _QGroundControl_, reconnect your GPS, and open QGC.
:::

Horizontal accuracy is an estimated distance in meters; it is not HDOP, which is dimensionless.
For live Remote ID, operator altitude must be WGS84 ellipsoid altitude. NMEA sources can provide
it using MSL altitude and geoid separation. If the datum or conversion is unavailable, QGC does
not send MSL altitude as ellipsoid altitude. FAA configurations requiring operator altitude report
the live position as unavailable; configurations permitting horizontal-only reporting send an
unknown altitude.

## ADSB Server {#adsb_server}

The settings are:

- **Connect to ADSB SBS server**: Check to connect to ADSB server on startup.
- **Host address**: Host address of ADSB server
- **Server port**: Port of ADSB server

QGC can consume ADSB messages in SBS format from a remote or local server (at the specified IP address/port) and display detected vehicles on the Fly View map.

::: tip
One way to get ADSB information from nearby vehicles is to use [dump1090](https://github.com/antirez/dump1090) to serve the data from a connected RTL-SDR dongle to QGC.

The steps are:

1. Get an RTL-SDR dongle (and antenna) and attach it to your ground station computer (you may need to find compatible drivers for your OS).
1. Install _dump1090_ on your OS (either pre-built or build from source).
1. Run `dump1090 --net` to start broadcasting messages for detected vehicles on TCP localhost port 30003 (127.0.0.1:30003).
1. Enter the server (`127.0.0.1`) and port (`30003`) address in the QGC settings above.
1. Restart QGC to start seeing local vehicles on the map.

:::

## Video {#video}

The _Video_ section is used to define the source and connection settings for video that will be displayed in _Fly View_.

The settings are:

- **Video Source**: Video Stream Disabled | RTSP Video Stream | UDP h.264 Video Stream | UDP h.265 Video Stream | TCP-MPEG2 Video Stream | MPEG-TS Video Stream | Integrated Camera

  ::: info
  If no video source is specified then no other video or _video recording_ settings will be displayed.
  :::

- **URL/Port**: Connection type-specific stream address (may be port or URL).
- **Aspect Ratio**: Aspect ratio for scaling video in video widget (set to 0.0 to ignore scaling)
- **Disabled When Disarmed**: Disable video feed when vehicle is disarmed.
- **Low Latency Mode**: Enabling low latency mode reduces the video stream latency, but may cause frame loss and choppy video (especially with a poor network connection). <!-- disables the internal jitter buffer -->

## Video Recording

The _Video Recording_ section is used to specify the file format and maximum allocated file storage for storing video.
Videos are saved to a sub-directory ("Video") of the [Application Load/Save Path](#load_save_path).

The settings are:

- **Auto-Delete Files**: If checked, files are auto deleted when the specified amount of storage is used.
- **Max Storage Usage**: Maximum video file storage before video files are auto deleted.
- **Video File Format**: File format for the saved video recording: mkv, mov, mp4.

## Brand Image

This setting specifies the _brand image_ used for indoor/outdoor colour schemes.

The brand image is displayed in place of the icon for the connected autopilot in the top right corner of the toolbar.
It is provided so that users can easily create screen/video captures that include a company logo/branding.

The settings are:

- **Indoor Image**: Brand image used in [indoor color scheme](#colour_scheme)
- **Outdoor Image**: Brand image used in [outdoor color scheme](#colour_scheme)
- **Reset Default Brand Image**: Reset the brand image back to default.
