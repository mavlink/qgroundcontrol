# GNSS Receiver

The _GNSS Receiver_ settings (**SettingsView > GNSS Receiver**) connect a GNSS receiver to the ground station over serial, TCP, UDP or Bluetooth.
_QGroundControl_ can set up a supported receiver as an RTK base station whose corrections reach vehicles, forward the corrections of a base station that is already set up, or use a receiver for the ground station position alone.
Corrections from NTRIP casters and UDP are configured in [RTK Corrections](ntrip_rtk.md).

The page follows the work from top to bottom: what the receiver is used as and what to connect, with **Connect**; what a base station does; what the receiver reports; a second receiver for the ground station position; and the ground station position itself.
Each group is a section of **GNSS Receiver** in the settings sidebar, and the settings search finds each one.

## What do you want to do? {#tasks}

The settings of a receiver that is connected are locked: select **Disconnect** to change them, then **Connect**.
To get corrections from an NTRIP caster instead of a receiver of your own, see [Use an NTRIP caster](ntrip_rtk.md#task_ntrip).

### Set up an RTK base station with a survey {#task_survey}

1. Connect the receiver to the ground station and open **GNSS Receiver**.
2. Leave **Use receiver as** on _RTK base station_, and **Receiver manufacturer** on _Detect automatically_ unless you want to name the receiver.
3. Select the **Receiver connection** and its device. With **Connect automatically** on, a serial device can stay on _Any RTK receiver on USB_.
4. Under [RTK Base Station](#base_station), leave **Survey-In** selected and set what the survey has to reach.
5. Select **Connect**, at the end of the first group. The line beside it says what the receiver is doing, and a message below it why a connection failed.
6. Watch the survey under [GNSS Receiver Status](#status). The base sends corrections once it has completed, and the [Corrections Status](ntrip_rtk.md#corrections-status) then names the _GNSS receiver_ as the source.
7. Select **Save Survey Position** to start from that position next time, with a **Site name** to keep it as a [saved base site](#base_sites).

### Start a base station from a saved position {#task_saved_position}

1. Select what to connect as for a survey, steps 1 to 3 above.
2. Under [RTK Base Station](#base_station), select **Fixed position**.
3. Under [Saved Base Sites](#base_sites), select the site and **Use Site**, or enter the coordinates of the position.
4. Enter the **Antenna height** when the position is that of a mark below the antenna.
5. Select **Connect**. For a u-blox or Trimble receiver, **Position check** in the [status](#status) then says how far the position is from the one the receiver found on its own.

### Forward the corrections of a base station that is already set up {#task_existing_base}

1. Set **Use receiver as** to _Existing base station_.
2. Select the **Receiver connection** and its device, and for a serial device the receiver's **Baud rate**, or _Auto_.
3. Select **Connect**. _QGroundControl_ sends the receiver nothing and forwards the RTCM corrections it sends; the status line reads _Existing base station connected; forwarding its RTCM_.
4. The [Corrections Status](ntrip_rtk.md#corrections-status) names the _GNSS receiver_ as the source once corrections arrive.

What such a receiver has to send is described in [Receivers that are not configured](#passive).

### Use a receiver only for the ground station position {#task_position_only}

Without a base station of your own:

1. Set **Use receiver as** to _Position only_.
2. Select the **Receiver connection** and its device, and for a serial device the receiver's **Baud rate**, or _Auto_.
3. Select **Connect**.
4. [Ground Station Position](#gcs_position), the last group, names the _GNSS receiver_ as the source in use once it has a fix.

Beside a receiver that is an RTK base station, whose position is that of its antenna:

1. Leave the GNSS receiver as it is.
2. Under [Position Receiver](#position_receiver), turn on **Use a position receiver**.
3. Select its **Receiver connection** and device.
4. Select **Connect** in that group, below its settings.
5. [Ground Station Position](#gcs_position) names the _Position receiver_ as the source in use once it has a fix.

## GNSS Receiver {#receiver}

The first group says what the receiver is used as and what to connect, and ends with the connection.
The settings are:

- **Use receiver as:** _RTK base station_ sets up a supported receiver as an RTK base station and forwards its corrections to vehicles.
  _Existing base station_ is for a receiver that is a base station already: _QGroundControl_ never configures it, and forwards the RTCM corrections it sends.
  _Position only_ uses a receiver's position output and forwards nothing.
  Both of the last two use the receiver's output as it is, as [Receivers that are not configured](#passive) describes.
  The choice stands for the **Receiver role** and **Forward receiver RTCM** settings of earlier versions and shows what they saved: _Configured base_ is _RTK base station_, and _Passive_ is _Existing base station_ with forwarding on and _Position only_ with it off.
- **Receiver manufacturer** (_RTK base station_ only): The receiver family QGroundControl configures as a base station.
  _Detect automatically_, the default, identifies the receiver each time it connects: QGroundControl listens to its output and sends read-only identification queries, at each rate the supported receivers use when **Baud rate** is _Auto_.
  Until then the settings of every supported receiver are shown, and connecting reports a base mode the identified receiver does not support.
  QGroundControl remembers the receiver it found on a serial device, TCP address or Bluetooth device, with its baud rates, and asks that receiver first the next time it connects there: a receiver that is still there connects without the search.
  When it does not answer within a second at a remembered rate, the search runs as before.
  The eight connections used last are remembered, and a [recorded](#receiver) connection always runs the search.
  Meanwhile the [status line](#status) names what is tried: _Identifying receiver: trying u-blox at 115200 baud, as last time..._ for the remembered receiver. For each rate of the search it shows _Identifying receiver at 38400 baud..._ while QGroundControl listens there, and then each manufacturer whose receivers it asks in turn, as in _Identifying receiver: trying Trimble at 38400 baud..._.
  Select a manufacturer to configure only that receiver family.
  A manufacturer saved by a version without receiver roles only filtered the settings shown, so it resets to _Detect automatically_.
- **Receiver connection:** _Serial_ connects a receiver on a local port, _TCP_ a receiver's network port or a serial-to-TCP bridge, and _UDP_ listens for the output of a receiver that is not configured.
  _Bluetooth_ connects a receiver, or a Bluetooth serial adapter in front of one, over the serial port profile of classic Bluetooth.
  Builds without serial port support connect a _Serial_ selection over TCP.
  _Bluetooth_ is offered where the platform has Bluetooth serial connections, which iOS has not.
  - **Receiver host** and **Receiver TCP port:** The receiver or bridge to connect to. A bridge in front of an RTK base station must already run the receiver link at 115200 baud; any other receiver's bridge runs at that receiver's baud rate.
  - **Receiver UDP port:** The local port the receiver's output arrives on. The first sender is used until it stops sending.
  - **Bluetooth device:** The receiver's Bluetooth device. **Scan for Devices** lists the devices it finds by name and address, and the one you choose is saved.
    The saved device stays first in the list, also when a scan does not find it.
    Scanning turns Bluetooth on when it is off, and where the platform asks for the Bluetooth permission, QGroundControl asks for it when it scans or connects.
    Pair a receiver that needs pairing in the operating system's Bluetooth settings first.
    A Bluetooth serial adapter in front of an RTK base station must already run the receiver link at 115200 baud, as a TCP bridge must; any other receiver's adapter runs at that receiver's baud rate.
    A receiver that offers only Bluetooth Low Energy has no serial port profile and cannot be connected this way.
  - **Serial device:** The receiver's port. The list names each port by its USB description, such as _u-blox GNSS receiver – /dev/ttyACM0_, and marks ports another connection, such as a vehicle link, uses as _(in use)_.
    Where only one serial device can be open at a time, as with Android's USB serial driver, the other ports are marked _(another is in use)_ while another connection has one open, and **Connect** says so: disconnect that connection first.
    A saved device that is not plugged in is marked _(unavailable)_.
    A mark comes before the name, such as _(in use) u-blox GNSS receiver – /dev/ttyACM0_, so that it stays readable where the list is too narrow for a long name.
    For an RTK base station with **Connect automatically** on, _Any RTK receiver on USB_ connects to the first receiver whose USB adapter is a known RTK receiver.
    Where only one serial device can be open at a time, it connects no receiver while another connection has a device open, and the [receiver status](#status) says so for a receiver that is plugged in.
  - **Baud rate:** The serial rate. _Auto_ lets an RTK base station detect the rate, and [finds the rate](#passive) of a receiver that is not configured by listening. _Custom_ accepts any other rate.
- **Connect automatically:** On by default, and beside the device, which it can leave to USB discovery. QGroundControl connects the saved receiver when it starts, reconnects it with increasing delays when the connection is lost, and reconnects a serial receiver as soon as it is plugged back in.
  Settings that cannot connect, such as a base mode the selected receiver does not support, are reported once and not retried until you change them.
  The changed settings then connect on their own, also when **Connect** was refused for them in the meantime.
  With _Any RTK receiver on USB_ selected, it connects to an RTK receiver detected on USB instead.
  **Disconnect** pauses automatic connection until you select **Connect** again or turn the setting back on.
  When off, the receiver connects only when you select **Connect**, and a lost connection is only reported.
  Automatic connections never save settings to the receiver's flash.
- **Record receiver data:** Records every connection that starts while it is on, to send with a problem report.
  It is off each time QGroundControl starts, and a receiver that is already connected is recorded from its next connection.
  Each connection gets a file in the `GPSReceiver` folder of the [Application Load/Save Path](general.md#load_save_path), named with the UTC time it started and the receiver type, such as `20260102T030405.006Z-ublox.log`.
  The file's path is shown below the switch while the connection lasts.
  The file holds the raw receiver input and output: every byte the receiver sent and every byte QGroundControl sent to it, with the baud rate changes.
  It starts with the settings the connection was made with: the receiver type, the baud rate, the corrections format and the constellations in them when some are left out, the permission to save to the receiver's flash, and the base mode with its survey limits or fixed position.
  It therefore includes the receiver's positions and the base station location, so share it only where that is acceptable.
  It is a text file: after a heading line, each line is the time in microseconds since the recording started, then `SETTINGS` and the connection's settings, `RX` and the bytes received in hexadecimal, `TX` and the bytes sent, `BAUD` and the new baud rate, or, for what the connection refused, `TX-REFUSED` with the reason and the bytes that were not sent, or `BAUD-REFUSED` with the baud rate.
  A recording stops when its file reaches 256 MB or cannot be written: the application log reports it and the path is no longer shown. The connection continues either way.
- **Allow flash save and restart** (Quectel): For the next connection only, lets QGroundControl save base settings to the receiver's flash and restart it.
  With _Detect automatically_, the permission and its explanation are behind the row **Flash save for Quectel receivers**: **Show** opens them and **Hide** closes them and withdraws a permission that was given.
  They open without being asked once a receiver has refused a connection for want of the permission.
  Leaving the page withdraws the permission too, so it is never used by the **Connect** of the toolbar's drop-down.

### Connect {#connect}

The group ends with the connection: a dot for its state, a line that says what the receiver is doing, and the button that connects or disconnects it.
The dot is grey without a receiver, orange while a connection is opened or awaited, green once the receiver is connected, and red after a connection failed or was lost; it turns orange for a connected receiver that has a problem.
The button reads **Connect**, then **Disconnect** from the moment a connection is being opened, and **Cancel reconnect** while QGroundControl waits to reconnect automatically.
**Connect** uses the settings as they are, and is unavailable while they select no receiver type, a base mode the receiver does not support, or UDP for an RTK base station.
A message below that line explains a failed or lost connection.
When **Connect** cannot start, as without a TCP host or with a serial device that is not plugged in, the message says why, and it goes away once the settings no longer have that problem.
When a receiver identified itself before its configuration failed, the message names its model and firmware.
When a u-blox receiver reports that it produces more output than its connection carries, the message warns until 30 seconds after the last report: use a higher baud rate or turn on **Compact RTCM corrections (MSM4)**.

## RTK Base Station {#base_station}

The group is shown for an _RTK base station_.
The base can survey its own position, or start from a known position for the base station.
Its corrections are sent while the receiver reports a valid base position: once the survey completes, or once the receiver uses the fixed position.
Until then, and whenever the receiver stops reporting it, the [receiver status](#status) says that the base sends no corrections.
A Septentrio receiver is the exception: its corrections are sent as soon as it is configured.
The corrections pass the [RTCM message filter](ntrip_rtk.md#correction-routing) before they reach vehicles.

The settings are:

- **Base mode**, as the receiver supports it:
  - **Survey-In:** The receiver surveys its own position.
    **Survey-In accuracy** (u-blox) is the position accuracy the survey must reach, and **Min observation time** how long it lasts at least.
    A Quectel receiver shows **Observation accuracy limit**, which filters each 1 Hz observation, and **Accepted observation time**, which counts the accepted observations.
  - **Fixed position:** The base starts from a known position: **Base Position Latitude**, **Base Position Longitude**, **Base Position Alt (WGS84)** and, for u-blox, **Base Position Accuracy**.
    **Base position format** shows the same position as _ECEF X/Y/Z_ instead, in meters with 0.1 mm, as a surveyor's or a reference station's data sheet gives it: **Base Position X (ECEF, m)**, **Y** and **Z**.
    One position is saved, as latitude, longitude and height, and both formats show it: a value you only look at does not change, and a coordinate you edit moves the position by that much.
    While X, Y and Z are not a position near the Earth's surface, as after the first of three new coordinates, a note says that the base position is unchanged.
    **Antenna height** is how far the antenna's reference point, usually the bottom of its mount, is above that position, measured vertically.
    Enter it when the position is that of a mark on the ground and the antenna stands above it on a tripod or a mast: the receiver is then given the antenna's position, the base position's height plus the antenna height, and the base announces that position to rovers.
    Leave it at zero when the position is the antenna's own, as a surveyed one is.
    [Saved base sites](#base_sites) keep several such positions under a name.
  - **Receiver-managed averaging:** The receiver averages its position for up to **Maximum averaging time**, without an accuracy guarantee (Unicore).
- **Constellations in corrections** (u-blox, Trimble and Unicore): The constellations whose observations the base sends to rovers.
  GPS is always sent. Clear _GLONASS_, _Galileo_, _BeiDou_ or, for a Trimble receiver, _QZSS_ to leave its observations out, for a slow telemetry link or for rovers that use fewer constellations.
  Rovers then fix with the constellations that remain, so leave out only what the link cannot carry.
  It selects the messages the base sends, not the satellites it tracks: the receiver keeps using every constellation for its own position.
  The other receivers send their own set. With _Detect automatically_ all four are offered, and a receiver that sends its own set does so whatever is selected, which the application log notes.
  **Constellations** in the [Corrections status](ntrip_rtk.md#corrections-status) shows what the base sends.
- **Compact RTCM corrections (MSM4)** (u-blox): Sends MSM4 instead of MSM7 observations, about a third less correction bandwidth for slow telemetry links.

The notes on a Quectel base, on its restart and on what its survey counts, are shown once the receiver is known to be one: selected as the manufacturer, or identified by _Detect automatically_ for the connection that runs.

A fixed position that is another place's, or has a wrong sign, makes the base send corrections that no rover can use, and the receiver does not show it: once it holds a fixed position, that is the only one it reports.
A u-blox or Trimble receiver is therefore given the fixed position only after it found a position of its own, which QGroundControl compares with it:

- A u-blox receiver is waited for up to 30 seconds, while the [status line](#status) reads _Waiting for the receiver's own position, to check the fixed position against it..._.
  A receiver without a view of the sky finds none, so after that time the base starts unchecked.
  A Trimble receiver starts as a base from its first position in any case.
- The receiver status then shows **Position check**: how far the fixed position is from the receiver's own, or _Not checked: the receiver found no position of its own_.
- More than 50 m apart is too far for one place. QGroundControl then says so once, and the message stays below the connection line: _The fixed base position is 12.4 km from the position the receiver found on its own_.
  It only warns: the base runs and sends its corrections, as you may know the position better than a receiver without corrections does, which finds itself within a few meters.
  Check the coordinates and their signs, and that the [saved site](#base_sites) is this one.

The other receivers are not compared this way.
A Septentrio or Femtomes receiver reports only the position it was given, and QGroundControl reads back from a Unicore or Quectel receiver that it uses the position.

Connecting configures the receiver again, which restarts a survey.
So when a base stops sending, QGroundControl reports after a few seconds that no data arrives and keeps the connection for 30 seconds.
Only if the receiver stays silent that long does the connection end and, with **Connect automatically**, get made again.

::: info
The _Survey-In_ process is a startup procedure required by RTK GPS systems to get an accurate estimate of the base station position.
The process takes measurements over time, leading to increasing position accuracy.
Both of the setting conditions must be met for the Survey-In process to complete.
For more information see [RTK GPS](https://docs.px4.io/en/advanced_features/rtk-gps.html) (PX4 docs) and [GPS- How it works](http://ardupilot.org/copter/docs/common-gps-how-it-works.html#rtk-corrections) (ArduPilot docs).
:::

::: tip
Survey-In is time consuming, so save its result: when a survey completes, the receiver status offers **Save Survey Position**.
Select **Fixed position** to start the base from the saved position next time.
A survey finds where the antenna is, so saving it sets **Antenna height** to zero.
The values persist across QGC reboots until they are changed.
:::

### Saved Base Sites {#base_sites}

The fixed position settings hold one base position, which **Save Survey Position** replaces.
To work from several places, keep each position as a named site:

- When a survey completes, enter a **Site name** before you select **Save Survey Position**.
  The surveyed position becomes the fixed base position and is also kept under that name.
- With **Fixed position** selected, enter a name under **Save base position as site** and select **Save Site** to keep the base position the settings show, with its **Antenna height**.
- Under **Saved base sites**, select a site and **Use Site** to make it the fixed base position, with the antenna height it was saved with, or **Delete Site** to remove it.
  A site saved by a version without antenna heights has none.
  Selecting a site changes nothing until you use it.
  A connected base keeps the position it started from, so disconnect the receiver to use a site.

Saving under a name that is already saved replaces that site.
Up to 50 sites are kept; when the list is full, delete a site before you save another, or save a survey without a site name.

## Receivers that are not configured {#passive}

_Existing base station_ and _Position only_ use the receiver's existing output without configuring it or sending it anything, over serial, TCP, UDP or Bluetooth:

- The receiver may output standard NMEA, u-blox UBX or Septentrio SBF; QGC identifies the protocol from the data and shows it as _Detected_ in the receiver status.
  When a receiver sends both NMEA and a binary protocol, positions come from the one recognized first, until it stops reporting positions.
- With NMEA, the position comes from the GGA sentence, or from the GNS sentence of a receiver that sends that instead.
  RMC or VTG add the speed and heading, and GST or GSA the accuracy.
  A position that the receiver marks as dead-reckoned is shown with the fix type _DR_, not as a lost fix.
  The position of a receiver that sends RMC or GST after GGA is used once it has sent them, which at 4800 baud can be most of a second later; a higher baud rate shortens that.
- With _Existing base station_, the RTCM corrections the receiver sends are forwarded to vehicles. With _Position only_, nothing is forwarded.
- Select the connection (**Serial** with the receiver's baud rate, **TCP**, **UDP** with the port QGC listens on, or **Bluetooth** with the receiver's device).
  With the baud rate on **Auto**, QGC listens at 115200, 9600, 38400, 4800, 57600, 19200, 230400, 460800 and 921600 baud, for up to 2.2 seconds each, until NMEA, UBX, SBF or RTCM data arrives.
  It sends nothing to the receiver, and the receiver status shows the rate it found as _Receiver_.
  The status reads _Connecting to receiver..._ while QGC listens.
  When no rate carries such data, connecting fails after about 20 seconds and names the rates that were tried.
  Listening can miss a receiver that reports less often than every two seconds, so select the rate of such a receiver.

Such a receiver stays connected while it reports no position, and after a few seconds the [receiver status](#status) explains why: no data at all (wrong device, or another program uses it), data that isn't GNSS output (wrong device or baud rate), or a recognized protocol without position messages (enable NMEA GGA, u-blox NAV-PVT or Septentrio PVTGeodetic output).
A u-blox receiver that QGC configured as a base station keeps its USB NMEA output, so it also works as a _Position only_ receiver afterwards.

A separate _NMEA GPS Device_ from an earlier version becomes a _Position only_ receiver, unless RTK GPS auto-connect had been turned on explicitly or an RTK receiver's serial device was saved.
In that case the RTK base station stays the receiver; select the NMEA device again with **Use receiver as** on _Position only_ to use it instead.

## GNSS Receiver Status {#status}

The status is shown while a connection is open or being opened, below the settings.
It shows the receiver's fix and satellites, and any jamming or spoofing it reports.
The status line of a base station, beside [Connect](#connect), says when the base sends no corrections.
While a survey or the receiver's own averaging runs, it sends none until that completes.
Otherwise the status reads _Waiting for a valid base position; the base sends no corrections until then_, as before the receiver confirms a fixed position or after it loses its position.
[Corrections are sent](#base_station) once the base position is valid.
A Septentrio base is the exception: its corrections do not wait for the base position, so its status does not say this.
While a base surveys its position, the status shows how far the survey is, as far as the receiver reports that:

- A u-blox receiver reports the survey's **Duration** and **Current Accuracy**. Each is shown with what it has to reach, the minimum observation time and the survey accuracy, such as _95 of 180 s_, and with a bar. The survey ends once both are reached.
- A Quectel receiver reports its **Accepted observation time**, shown with the configured one and a bar, and its **Current Accuracy**. The accuracy has nothing to reach, because the receiver applies its limit to each observation.
- Trimble, Septentrio and Femtomes receivers average their position and report only when they are done. **Averaging** shows the time since QGroundControl started the averaging, and for a Trimble receiver also the time it was asked to average for. There is no bar, because the receiver reports no progress.
- A Unicore receiver reports no time either, so only the status line says that it averages.

Once the receiver reports a valid base position, the status shows it as **Base Position Latitude**, **Base Position Longitude** and **Base Antenna Alt (WGS84)**: the result of the survey or averaging, or the fixed position the base was given.
It is where the antenna is, so for a fixed position with an **Antenna height** its height is that much above the **Base Position Alt (WGS84)** of the settings.
A Trimble receiver reports the position it averaged without a height.
While the vehicle reports no GPS, the toolbar GNSS indicator shows the receiver, and a base station in a word: _Survey_ while it determines its position, _Waiting_ while it has no valid base position and sends no corrections, and _Base_ otherwise.
An identified receiver family or output protocol is shown as _Detected_.
**Constellations** lists the satellites of each satellite system in view as used/in view, such as _GPS 8/10, Galileo 6/7_; a dash replaces the used count when the receiver does not report which satellites it uses.
The row is not shown when the receiver does not report which system its satellites belong to.
**Satellite Signals** shows the satellites one by one, for a receiver that describes them: u-blox receivers do (UBX NAV-SAT, or NAV-SVINFO on older receivers), and so does any receiver that sends NMEA GSV.
**Show** opens the view and **Hide** closes it; it starts open on the settings page and closed in the narrower drop-down of the GNSS indicator.
Each satellite has a bar as tall as its signal is strong (C/N0, a full bar at 50 dB-Hz), after the name of its satellite system and in that system's color.
The sky plot places each satellite by azimuth and elevation, with north at the top, the horizon as the rim and the point overhead in the center; its rings mark 30 and 60 degrees of elevation.
A filled bar or marker is a satellite the receiver uses in its solution.
A receiver that does not report this, as one that sends GSV without GSA, shows outlines only.
The **Antenna** is shown as _OK_, _Open_ or _Short_ when the receiver supervises its antenna and reports it (u-blox MON-RF or MON-HW, also from a receiver that is not configured); an open or shorted antenna turns the GNSS indicator orange, as jamming does.

## Position Receiver {#position_receiver}

A receiver that is set up as an [RTK base station](#base_station) reports the position of its antenna.
With the base on a tripod, the ground station position is then the tripod's, and so is what uses it: _Follow Me_, the home position set at the ground station and the operator location of [Remote ID](remote_id.md).
**Use a position receiver** connects a second receiver that only provides the ground station position, such as the operator's own receiver beside the ground station.

- QGC never configures the position receiver and sends it nothing.
  It reads the output a [receiver that is not configured](#passive) has: standard NMEA, u-blox UBX or Septentrio SBF, over **Serial** (with the receiver's baud rate, or **Auto**), **TCP**, **UDP** or **Bluetooth**.
  _Bluetooth_ connects a handheld receiver, or a Bluetooth serial adapter that runs at its receiver's baud rate, with the [device choice](#receiver) the GNSS receiver has, where the platform has Bluetooth serial connections.
  RTCM that it sends is not forwarded to vehicles.
- While the setting is on, the position receiver is the only receiver that provides the ground station position, whether or not it is connected or has a fix.
  The GNSS receiver's position is never used then.
  With the [Source](#gcs_position) on _Best available_, the position comes from this device until the position receiver has a fix; with _GNSS receiver_, there is no ground station position until then.
  The [Ground Station Position](#gcs_position) group names the source in use as _Position receiver_.
- The saved position receiver connects when QGC starts and reconnects when its connection is lost.
  **Disconnect** pauses that until you select **Connect** again, and turning the setting off disconnects it.
- An RTK base station that looks for an RTK receiver on USB never connects to the serial device selected for the position receiver, so the position receiver can be an RTK receiver on USB as well.
  Select different serial devices, and different UDP ports, for the two receivers.
- A Bluetooth device takes one connection. While the position receiver is in use on a Bluetooth device, that device is the position receiver's, connected or not: the GNSS receiver does not connect to it, says so in its status, and connects once the position receiver is turned off or set to another device.
- Its status, below its settings and in the toolbar's GNSS drop-down, has its own **Connect** and shows what the [receiver status](#status) shows for the GNSS receiver.
  **Record receiver data** records its connections to the `GPSPositionReceiver` folder.

The GNSS receiver stays the _GNSS receiver_ whose position [RTK Corrections](ntrip_rtk.md) can report to an NTRIP caster; the position receiver's position is reported as the _Ground station position_.

## Ground Station Position {#gcs_position}

_QGroundControl_ will automatically use an internal GPS to display its own location on the map with a purple `Q` icon (if the GPS provides a heading, this will be also indicated by the icon).
It may also use the GPS as a location source for _Follow Me Mode_ - currently supported on [PX4 Multicopters only](https://docs.px4.io/en/flight_modes/follow_me.html).

Vehicles take the altitudes they are sent as above mean sea level, so that is how _Follow Me_ and [Update return to home position based on device location](fly_view.md) send the ground station's altitude.
A GNSS receiver and macOS, iOS and Windows devices report it that way.
A height above the WGS84 ellipsoid, which is what Android devices report and what the fixed or surveyed position of a base station has, is converted with the EGM96 geoid model built into QGroundControl.
The converted altitude is within 1 m of the model almost everywhere, and up to 4 m off where the geoid changes over a short distance, as in northern Colombia, on the Canary Islands and on Hawaii.

An altitude of unknown reference, as the location service of a Linux device reports, or a position without an altitude is not sent. The vehicle's own home altitude is sent instead: home moves only horizontally, and a followed target is taken to be at home altitude.
The home position also keeps the vehicle's altitude when the ground station's is not accurate to 10 m.
A receiver that states no vertical accuracy, as an NMEA receiver without GST sentences, counts as that accurate while its VDOP is below 2.

[Remote ID](remote_id.md) reports the operator's height above the WGS84 ellipsoid instead.
It sends the ellipsoid height of a GNSS receiver or an Android device as it is, and converts an altitude above mean sea level, which is all macOS, iOS and Windows devices report, with the same geoid model.

A connected GNSS receiver can provide the ground station position instead; set [Use receiver as](#receiver) to _Position only_ for a receiver that only does that, or a [position receiver](#position_receiver) beside a receiver that is an RTK base station.
The settings are:

- **Source**: _Best available_ uses the GNSS receiver while it has a fix and falls back to this device; select _GNSS receiver_ or _This device_ to use only one.
  While a [position receiver](#position_receiver) is in use, it is the GNSS receiver meant here.
  **Using** names the source in use: _GNSS receiver_, _Position receiver_ or _This device_, the device _QGroundControl_ runs on.

A position is used while it is accurate to 100 m horizontally; a less accurate one is shown as _Position fix does not meet accuracy requirements_.
A receiver that states no horizontal accuracy, as an NMEA receiver without GST sentences, counts as accurate to 5.1 m times its HDOP, which is one standard deviation as a stated accuracy is, so its position is used while its HDOP is below 19.6.

## In the toolbar {#indicator}

The drop-down of the toolbar's GNSS indicator shows status and the actions that are needed in flight: the vehicle's GPS, the receiver status with its connection line and **Connect**, the position receiver's while one is in use, the corrections, the NTRIP connection once a caster is set up, and the ground station position.
Beside a vehicle's GPS status, the GNSS receiver is shown once it is set up, so a saved receiver connects without opening the settings.
Nothing is set up there: the last group has a row for **GNSS Receiver** and one for **RTK Corrections**, whose **Configure** button closes the drop-down and opens that settings page.
