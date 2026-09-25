# NTRIP / RTK

Configure RTCM correction sources for centimeter-level RTK GPS positioning: the built-in NTRIP
client, an RTK base receiver connected to QGC, and UDP RTCM input. QGC forwards the selected
corrections to connected vehicles over MAVLink.

## Connection Status

Shows the current NTRIP connection state (connected/connecting/disconnected) with the time since
the last correction, message and byte counters, the received RTCM message types, data rate, and a
Connect/Disconnect button. Dropped connections reconnect automatically with an increasing delay.
Rejected credentials and TLS certificate failures stop instead, because they need a settings
change: the button becomes **Retry**, and a separate **Disconnect** button stops further attempts.
If no position is available to send to the caster while corrections are not arriving, QGC warns
that network (VRS) mountpoints need a GGA position.

A connected socket does not prove that usable corrections are arriving. QGC separately monitors
valid RTCM traffic and reports a stale stream when no valid corrections arrive, even if the server
continues sending other bytes. Message filtering can also prevent valid corrections from being
forwarded. Stopping preserves the connection's cumulative counters; a new connection session
starts a fresh retry budget.

## Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — enable TLS for the NTRIP connection

## Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

Source-table discovery supports HTTP and legacy NTRIP source-table responses. A mountpoint with
missing or invalid coordinates has an unknown distance, rather than a fabricated location. When
**Browse** sends a username or password without TLS encryption, QGC warns that the credentials are
not encrypted.

## Options

- **RTCM Message Filter** — whitelist of RTCM message types to forward (empty = forward all)

### GGA position

Automatic GGA selection prefers vehicle GPS, then the vehicle's fused position, the connected RTK
receiver, and ground-station position, subject to each source's eligibility. Vehicle GPS and fused position have
independent freshness; receiving heartbeats does not make an old position current. Vehicle GPS
uses its own reported altitude rather than borrowing the fused position's altitude.

GGA requires mean-sea-level altitude. A receiver solution without mean-sea-level altitude is not
eligible without a known conversion. Actual fix quality and available DOP/satellite-use metadata are preserved; unknown
metadata is not replaced with nominal precision values.

## GNSS Receiver

Connect one GNSS receiver: an external NMEA GPS for the ground station position, a receiver whose
RTCM output QGC forwards, or an RTK base station that QGC configures. The same settings are
available from the GPS toolbar indicator's expanded page.

- **Receiver role** — how QGC uses the receiver:
  - _Position only_ — its NMEA output provides the [GCS position](comm_links.md#gcs-position); RTCM
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
  manual connection turns it off.
- **Connect on startup** — connect the saved receiver when QGC starts and keep retrying until it
  is available. If auto-connect is also on, a receiver that is missing at startup is left to
  auto-connect instead. Startup connections never turn auto-connect off or save settings to receiver
  flash.
- **Survey-In**, **Specify position**, or **Receiver-managed averaging** — how the base finds its
  position, depending on the receiver. See [RTK GPS](general.md#rtk_gps) for the survey-in and
  fixed-position settings.
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

The **GNSS Receiver Status** section on this page and the GPS toolbar indicator show the receiver's
fix, model and firmware (when the receiver reports them), connection, satellites, and survey-in
progress. The receiver's own
position solution is also available as the [GCS position](comm_links.md#gcs-position) and as the
_RTK Receiver_ GGA source. Base receivers report only a time fix once their position is fixed or surveyed, so QGC uses
the fixed position or completed survey-in position instead. That position has only ellipsoid height, so it is not
eligible as a GGA source. The base station is also marked on the Fly and Plan maps, and the indicator shows the
receiver's jamming and spoofing state when it reports them.

## Correction Routing

- **Automatic** selects the freshest usable correction stream.
- Choosing a specific source (_Local base station_, _NTRIP_, or _UDP_) forwards only that source.
  When a source has more than one stream, such as UDP from several senders, you can also pin one
  stream.

## Correction Diagnostics

Shows each correction stream's state, each source's received data rate, frame counters, and the
count of each validated RTCM message type received in the current session, and the bytes queued or
dropped for each vehicle link. Queued bytes were accepted by an output; they do not
confirm that the vehicle applied the corrections. Enable **Show recent correction events** for a
per-frame history.

## UDP Forwarding

- **UDP forward RTCM data** — forward received RTCM data to another device via UDP
- **UDP target address** — destination IP for forwarded data
- **UDP target port** — destination port for forwarded data

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM data from a UDP source instead of NTRIP
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)

QGC forwards corrections to vehicles over MAVLink and, for NTRIP, optionally to the UDP forwarding
target. Corrections are not written back to a connected receiver.
