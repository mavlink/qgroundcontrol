# NTRIP / RTK

Configure the built-in NTRIP client to stream RTCM correction data to the vehicle for centimeter-level RTK GPS positioning.

## Connection Status

Shows the current NTRIP connection state (connected/connecting/disconnected) with message and byte counters, data rate, and a Connect/Disconnect button.

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
missing or invalid coordinates has an unknown distance, rather than a fabricated location.

## Options

- **RTCM Message Filter** — whitelist of RTCM message types to forward (empty = forward all)

### GGA position

Automatic GGA selection prefers vehicle GPS, then the vehicle's fused position, RTK base, and
ground-station position, subject to each source's eligibility. Vehicle GPS and fused position have
independent freshness; receiving heartbeats does not make an old position current. Vehicle GPS
uses its own reported altitude rather than borrowing the fused position's altitude.

GGA requires mean-sea-level altitude. An ellipsoid-only RTK base is not eligible without a known
conversion. Actual fix quality and available DOP/satellite-use metadata are preserved; unknown
metadata is not replaced with nominal precision values.

## UDP Forwarding

- **UDP forward RTCM data** — forward received RTCM data to another device via UDP
- **UDP target address** — destination IP for forwarded data
- **UDP target port** — destination port for forwarded data

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM data from a UDP source instead of NTRIP
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)

QGC forwards corrections to MAVLink and UDP destinations. Android serial input and receiver
configuration do not imply support for bounded serial correction injection: that transport API
operation is unsupported and is not offered as an application output destination.
