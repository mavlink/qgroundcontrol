# NTRIP / RTK

Configure the built-in NTRIP client to stream RTCM correction data to the vehicle for centimeter-level RTK GPS positioning.

## Connection Status

Shows the current NTRIP connection state (connected/connecting/disconnected) with message and byte counters, data rate, and a Connect/Disconnect button.

## Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — enable TLS for the NTRIP connection

## Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

## Options

- **RTCM Message Filter** — whitelist of RTCM message types to forward (empty = forward all)

## UDP Forwarding

- **UDP forward RTCM data** — forward received RTCM data to another device via UDP
- **UDP target address** — destination IP for forwarded data
- **UDP target port** — destination port for forwarded data

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM data from a UDP source instead of NTRIP
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)
