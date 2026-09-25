# GNSS Receiver

Connect a GNSS receiver to the ground station, see its status, and choose where the ground station
position comes from. RTK correction sources, including NTRIP, are on the
[RTK Corrections](rtk_corrections.md) page.

## GNSS Receiver Status

Shows whether a receiver is connected, and when it is: its role or base mode, model and firmware
(when the receiver reports them), connection, fix, satellites, jamming and spoofing state, and
survey-in progress.

## GNSS Receiver

Connect one GNSS receiver: an external NMEA GPS for the ground station position, a receiver whose
RTCM output QGC forwards, or an RTK base station that QGC configures. The same settings are
available from the GPS toolbar indicator's expanded page.

- **Receiver role** — how QGC uses the receiver:
  - _Position only_ — its NMEA output provides the [GCS position](#gcs-position); RTCM
    output is ignored. QGC never configures the receiver.
  - _Passive RTCM/NMEA_ — as _Position only_, and its RTCM output is forwarded to vehicles.
  - _Configured base_ — QGC configures a supported receiver as an RTK base station.
- **Receiver / settings** (configured base only) — the receiver family. Select a specific receiver
  to connect manually.
- **Receiver connection** — _Serial_ for a receiver on a local port, _TCP_ for a receiver's network
  port or a serial-to-TCP bridge, or _UDP_ to listen for NMEA/RTCM datagrams. UDP only receives
  data, so a configured base needs serial or TCP. Platforms without serial support connect a serial
  selection over TCP.
- **Serial device** and **Baud rate** — the receiver's port. _Auto_ lets QGC detect the rate of
  receivers it configures; the other roles need the rate already configured on the receiver.
- **Receiver host** and **Receiver TCP port** — the TCP endpoint. A serial-to-TCP bridge must
  already run the receiver link at 115200 baud, because QGC cannot change a bridge's rate.
- **Receiver UDP port** — the local port that receives datagrams (default: 14401). The first
  sender is used until it stops sending for five seconds.
- **Auto-connect known serial receivers** (configured base only) — automatically connect
  recognized serial RTK receivers. Auto-connect is not used while TCP or UDP is selected, and a
  manual connection turns it off. This is the only place the setting appears.
- **Connect on startup** — connect the saved receiver when QGC starts and keep retrying until it
  is available. If auto-connect is also on, a receiver that is missing at startup is left to
  auto-connect instead. Startup connections never turn auto-connect off or save settings to receiver
  flash.
- **Survey-In**, **Specify position**, or **Receiver-managed averaging** — how the base finds its
  position, depending on the receiver. See [Base Position](#base-position).
- **Compact RTCM corrections (MSM4)** (u-blox only) — send MSM4 instead of MSM7 observations. This
  uses about a third less correction bandwidth, for example on slow telemetry radios, but omits
  Doppler and uses lower measurement resolution.
- **Allow flash save and restart** (Quectel only) — for one connection, allow QGC to save base
  settings to the receiver and restart it.

A receiver that QGC does not configure stays connected while it is silent; its position simply
becomes stale. After a manual connection has worked once, QGC reconnects automatically if the receiver connection
is lost (for example, a dropped TCP bridge or an unplugged cable), retrying with an increasing delay
of up to 30 seconds. A replugged serial receiver is retried immediately. Press **Disconnect**, change
the connection settings, or enable auto-connect to stop reconnecting. Automatic attempts never reuse
flash-save permission.

The **GNSS Receiver Status** section and the GPS toolbar indicator show the receiver's
fix, model and firmware (when the receiver reports them), connection, satellites, and survey-in
progress. The receiver's own
position solution is also available as the [GCS position](#gcs-position) and as the
_RTK Receiver_ GGA source. Base receivers report only a time fix once their position is fixed or surveyed, so QGC uses
the fixed position or completed survey-in position instead. That position has only ellipsoid height, so it is not
eligible as a GGA source. The base station is also marked on the Fly map, and the indicator shows the
receiver's jamming and spoofing state when it reports them.

### Base Position

For a _Configured base_, these settings choose how the base finds its position: a survey-in, a known position you enter, or averaging managed by the receiver.

::: info
The _Survey-In_ process is a startup procedure required by RTK GPS systems to get an accurate estimate of the base station position.
The process takes measurements over time, leading to increasing position accuracy.
Survey controls depend on the receiver. U-blox supports an accuracy target and a minimum elapsed
observation duration. Quectel duration counts accepted observations and its accuracy setting filters
observations. Unicore uses receiver-managed averaging with a maximum duration, not a minimum
duration or an accuracy guarantee. Receivers without implemented duration control do not expose it.
For more information see [RTK GPS](https://docs.px4.io/en/advanced_features/rtk-gps.html) (PX4 docs) and [GPS- How it works](http://ardupilot.org/copter/docs/common-gps-how-it-works.html#rtk-corrections) (ArduPilot docs).
:::

::: tip
In order to save and reuse a base position (because Survey-In is time consuming!) perform Survey-In once, select _Specify position_ and press **Save Current Base Position** to copy in the values for the last survey.
The values will then persist across QGC reboots until they are changed.
:::

The settings are:

- **Survey-In**
  - **Survey-in accuracy (U-blox only):** The minimum position accuracy for the RTK Survey-In process to complete.
  - **Observation duration:** Receiver-specific elapsed time, accepted-observation time, or maximum averaging time.
- **Specify position**
  - **Base Position Latitude:** Latitude of fixed RTK base station.
  - **Base Position Longitude:** Longitude of fixed RTK base station.
  - **Base Position Alt (WGS84):** Altitude of fixed RTK base station.
  - **Base Position Accuracy:** Accuracy of base station position information.
  - **Save Current Base Position** (button): Press to copy settings from the last Survey-In operation to the _Specify position_ fields above.

Fixed-base altitude is ellipsoid height, not height above mean sea level. Use WGS84 coordinates
and retain sufficient decimal precision for the required RTK accuracy.
Receiver settings that require persistent writes or a restart need explicit consent; connecting
does not imply permission to change persistent configuration.
Corrected positions whose datum cannot be established are rejected rather than assumed to be WGS84.

## GCS Position

Choose where the ground station position (used for the map, Follow Me, NTRIP GGA, and Remote ID)
comes from:

- **Automatic** — the connected GNSS receiver, then this device's positioning. QGC switches to this
  device when the receiver loses its fix, and switches back after the receiver has been healthy for
  five seconds.
- **GNSS receiver** or **This device** — use only that source.

The section also shows the source in use, its status, and the current position. The GPS toolbar
indicator and the [Remote ID](remote_id.md#gcs-position) page show the source in use without
changing it.
