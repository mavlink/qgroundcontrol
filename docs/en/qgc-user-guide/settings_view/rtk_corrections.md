# RTK Corrections

Configure RTCM correction sources for centimeter-level RTK GPS positioning: the built-in NTRIP
client, UDP RTCM input, and a base receiver connected on the [GNSS Receiver](gnss_receiver.md)
page. QGC forwards the selected corrections to connected vehicles over MAVLink, and optionally to
another application over UDP.

## Corrections Status

Shows which correction source and stream vehicles receive, its data rate, and the NTRIP connection
state. The GPS toolbar indicator's dropdown shows the same summary while a correction source is
active.

## NTRIP Connection

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

## NTRIP Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — enable TLS for the NTRIP connection
- **Accept self-signed TLS certificates** — allow a caster whose certificate is self-signed

## NTRIP Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

Source-table discovery supports HTTP and legacy NTRIP source-table responses. A mountpoint with
missing or invalid coordinates has an unknown distance, rather than a fabricated location. When
**Browse** sends a username or password without TLS encryption, QGC warns that the credentials are
not encrypted.

## GGA Position Reporting

Network (VRS) mountpoints need the rover's approximate position, sent to the caster in NMEA GGA
sentences.

- **GGA Position Source** — the position reported to the caster; _Auto_ selects the best available
  source.
- **GGA Send Interval** — how often the position is sent.

Automatic GGA selection prefers vehicle GPS, then the vehicle's fused position, the connected RTK
receiver, and ground-station position, subject to each source's eligibility. Vehicle GPS and fused position have
independent freshness; receiving heartbeats does not make an old position current. Vehicle GPS
uses its own reported altitude rather than borrowing the fused position's altitude.

GGA requires mean-sea-level altitude. A receiver solution without mean-sea-level altitude is not
eligible without a known conversion. Actual fix quality and available DOP/satellite-use metadata are preserved; unknown
metadata is not replaced with nominal precision values.

## NTRIP Message Filter

- **RTCM Message Filter** — whitelist of RTCM message types to forward from the NTRIP caster
  (empty = forward all)

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM data from a UDP source instead of NTRIP
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)
- **UDP RTCM enable validation** — drop data that is not valid RTCM

## Correction Routing

- **Automatic** selects the freshest usable correction stream, preferring the local base station,
  then NTRIP, then UDP.
- Choosing a specific source (_Local base station_, _NTRIP_, or _UDP_) forwards only that source.
  When a source has more than one stream, such as UDP from several senders, you can also pin one
  stream.

## UDP Forwarding

Forward the stream selected for vehicles to another application over UDP, whichever source it
comes from.

- **Forward corrections over UDP** — enable forwarding
- **UDP output address** — destination IP address
- **UDP output port** — destination port

QGC does not forward to its own UDP RTCM input port, which would send the stream back to itself.
Corrections are not written back to a connected receiver.

## Correction Diagnostics

Shows each correction stream's state, each source's received data rate, frame counters, and the
count of each validated RTCM message type received in the current session, and the bytes queued or
dropped for each vehicle link. Queued bytes were accepted by an output; they do not
confirm that the vehicle applied the corrections. Enable **Show recent correction events** for a
per-frame history.
