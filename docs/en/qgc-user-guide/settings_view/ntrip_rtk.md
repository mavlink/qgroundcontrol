# NTRIP / RTK

Configure where RTK corrections come from and how they reach vehicles: the built-in NTRIP client, a local base station, or RTCM received over UDP.

## Corrections Status

Shows the correction stream currently sent to vehicles, its data rate, and the NTRIP connection state.
The toolbar GNSS indicator also offers **Connect** once an NTRIP server is configured.

## NTRIP Connection

Shows the NTRIP connection state with message and byte counters, the age of the last correction, and a Connect/Disconnect button.

## NTRIP Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — connect with TLS 1.2 or later. The first successful connection records the caster's certificate; a later connection that presents a different certificate fails with _certificate changed_. Turning off **Accept self-signed TLS certificates** forgets the recorded certificate.

## NTRIP Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

## GGA Position Reporting

Network RTK (VRS) casters need the rover position. Select the position QGC reports and how often.

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM corrections over UDP, for example from another application. UDP input is used alongside NTRIP and a local base station, and incoming data is always checked to be valid RTCM.
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)

## UDP Forwarding

- **UDP forward RTCM data** — forward the stream selected for vehicles to another device or application
- **UDP target address** — destination IP for forwarded data
- **UDP target port** — destination port for forwarded data

## Correction Routing

- **Vehicle correction source** — _Highest priority_ sends one fresh stream to vehicles, preferring the local base station, then NTRIP, then UDP, and switches when the preferred stream stops. Selecting a source category uses only that category, without falling back to another.
- **Stream** — shown when the selected category has several streams, for example two UDP senders. A pinned stream waits if it becomes unavailable.

Vehicles and UDP forwarding always receive the same stream.

## NTRIP Message Filter

- **RTCM Message Filter** — whitelist of RTCM message types to forward from NTRIP (empty = forward all)

## Correction Diagnostics

Per-source counters and, when enabled, a history of stream switches and dropped frames with the reason for each drop.
