# GNSS Receiver

The _GNSS Receiver_ settings (**SettingsView > GNSS Receiver**) connect an external GNSS receiver over serial, TCP or UDP.
_QGroundControl_ can configure a supported receiver as an RTK base station whose corrections reach vehicles, or use a receiver's existing output for the ground station position.
Corrections from NTRIP casters and UDP are configured in [RTK Corrections](ntrip_rtk.md).

## GNSS Receiver Status {#status}

The receiver status shows the connection, the receiver's fix and satellites, and any jamming or spoofing it reports.
An identified receiver family or output protocol is shown as _Detected_.
The **Antenna** is shown as _OK_, _Open_ or _Short_ when the receiver supervises its antenna and reports it (u-blox MON-RF or MON-HW, also from a passive receiver); an open or shorted antenna turns the GNSS indicator orange, as jamming does.

## GNSS Receiver {#receiver}

The settings are:

- **Receiver role:** _Configured base_ sets up a supported receiver as an RTK base station. _Passive_ uses a receiver's existing output without configuring it.
- **Forward receiver RTCM** (_Passive_ only): Sends the receiver's RTCM output to vehicles, for a receiver that is already a base station.
- **Receiver manufacturer** (_Configured base_ only): The receiver family QGroundControl configures as a base station.
  _Detect automatically_, the default, identifies the receiver each time it connects: QGroundControl listens to its output and sends read-only identification queries, at each rate the supported receivers use when **Baud rate** is _Auto_.
  Until then the settings of every supported receiver are shown, and connecting reports a base mode the identified receiver does not support.
  Select a manufacturer to configure only that receiver family.
  A manufacturer saved by a version without receiver roles only filtered the settings shown, so it resets to _Detect automatically_.
- **Receiver connection:** _Serial_ connects a receiver on a local port, _TCP_ a receiver's network port or a serial-to-TCP bridge, and _UDP_ listens for the output of a passive receiver.
  Builds without serial port support connect a _Serial_ selection over TCP.
  - **Receiver host** and **Receiver TCP port:** The receiver or bridge to connect to. A bridge must already run the receiver link at 115200 baud.
  - **Receiver UDP port:** The local port the receiver's output arrives on. The first sender is used until it stops sending.
  - **Serial device:** The receiver's port. The list names each port by its USB description, such as _u-blox GNSS receiver – /dev/ttyACM0_, and marks ports another connection, such as a vehicle link, uses as _(in use)_.
    For a configured base with **Connect automatically** on, _Any RTK receiver on USB_ connects to the first receiver whose USB adapter is a known RTK receiver.
  - **Baud rate:** The serial rate. _Auto_ lets a configured base detect the rate; a passive receiver needs the rate it already uses. _Custom_ accepts any other rate.
- **Base mode** (_Configured base_ only), as the receiver supports it:
  - **Survey-In:** The receiver surveys its own position.
    **Survey-In accuracy** (u-blox) is the position accuracy the survey must reach, and **Min observation time** how long it lasts at least.
    A Quectel receiver shows **Observation accuracy limit**, which filters each 1 Hz observation, and **Accepted observation time**, which counts the accepted observations.
  - **Fixed position:** The base starts from a known position: **Base Position Latitude**, **Base Position Longitude**, **Base Position Alt (WGS84)** and, for u-blox, **Base Position Accuracy**.
  - **Receiver-managed averaging:** The receiver averages its position for up to **Maximum averaging time**, without an accuracy guarantee (Unicore).
- **Compact RTCM corrections (MSM4)** (u-blox): Sends MSM4 instead of MSM7 observations, about a third less correction bandwidth for slow telemetry links.
- **Allow flash save and restart** (Quectel): For the next connection only, lets QGroundControl save base settings to the receiver's flash and restart it.
- **Connect automatically:** On by default. QGroundControl connects the saved receiver when it starts, reconnects it with increasing delays when the connection is lost, and reconnects a serial receiver as soon as it is plugged back in.
  With _Any RTK receiver on USB_ selected, it connects to an RTK receiver detected on USB instead.
  **Disconnect** pauses automatic connection until you select **Connect** again or turn the setting back on.
  When off, the receiver connects only when you select **Connect**, and a lost connection is only reported.
  Automatic connections never save settings to the receiver's flash.
- **Connect** / **Disconnect** (button): Connects the receiver with these settings, or disconnects it.

A message below the settings, also shown in the GNSS indicator's drop-down, explains a failed or lost connection.
When a u-blox receiver reports that it produces more output than its connection carries, the message warns until 30 seconds after the last report: use a higher baud rate or turn on **Compact RTCM corrections (MSM4)**.

### RTK Base Station {#base_station}

A configured base can survey its own position, or start from a known position for the base station.

::: info
The _Survey-In_ process is a startup procedure required by RTK GPS systems to get an accurate estimate of the base station position.
The process takes measurements over time, leading to increasing position accuracy.
Both of the setting conditions must be met for the Survey-In process to complete.
For more information see [RTK GPS](https://docs.px4.io/en/advanced_features/rtk-gps.html) (PX4 docs) and [GPS- How it works](http://ardupilot.org/copter/docs/common-gps-how-it-works.html#rtk-corrections) (ArduPilot docs).
:::

::: tip
Survey-In is time consuming, so save its result: when a survey completes, the receiver status offers **Save Survey Position**.
Select **Fixed position** to start the base from the saved position next time.
The values persist across QGC reboots until they are changed.
:::

### Passive Receiver {#passive}

An external GNSS receiver connected over serial, TCP or UDP can provide the ground station position.
The _Passive_ role uses the receiver's existing output without configuring it or sending it anything:

- The receiver may output standard NMEA, u-blox UBX or Septentrio SBF; QGC identifies the protocol from the data and shows it as _Detected_ in the receiver status.
  When a receiver sends both NMEA and a binary protocol, positions come from the one recognized first, until it stops reporting positions.
- Turn off **Forward receiver RTCM** to use the receiver only for the ground station position.
- Select the connection (**Serial** with the receiver's existing baud rate, **TCP**, or **UDP** with the port QGC listens on).

A passive receiver stays connected while it reports no position, and after a few seconds the message below the settings explains why: no data at all (wrong device, or another program uses it), data that isn't GNSS output (wrong device or baud rate), or a recognized protocol without position messages (enable NMEA GGA, u-blox NAV-PVT or Septentrio PVTGeodetic output).
A u-blox receiver that QGC configured as a base station keeps its USB NMEA output, so it also works as a passive receiver afterwards.

A separate _NMEA GPS Device_ from an earlier version becomes a passive receiver, unless RTK GPS auto-connect had been turned on explicitly or an RTK receiver's serial device was saved.
In that case the RTK base station stays the receiver; select the NMEA device again with the _Passive_ role to use it instead.

## GCS Position {#gcs_position}

_QGroundControl_ will automatically use an internal GPS to display its own location on the map with a purple `Q` icon (if the GPS provides a heading, this will be also indicated by the icon).
It may also use the GPS as a location source for _Follow Me Mode_ - currently supported on [PX4 Multicopters only](https://docs.px4.io/en/flight_modes/follow_me.html).

A connected GNSS receiver can provide the ground station position instead; use the [Passive](#passive) role for a receiver that only does that.
The settings are:

- **Source**: _Best available_ uses the GNSS receiver while it has a fix and falls back to this device; select _GNSS receiver_ or _This device_ to use only one.
