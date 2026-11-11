# RTK Corrections

Configure where RTK corrections come from and how they reach vehicles: the built-in NTRIP client, a local base station, or RTCM received over UDP.

## Corrections Status

Shows the correction stream currently sent to vehicles and its data rate, whether it comes from NTRIP, the local base station, or UDP.
**Message Types** lists the RTCM message types of that stream and how many messages of each have arrived, such as `1077 × 120, every 1 s`. The counts start again when vehicles receive another stream, or when the stream resumes after it stopped for more than 5 seconds.
Once a second message of a type has arrived, its entry also says how often the type arrives: in whole seconds (`every 10 s`), or in messages per second (`5 Hz`) from two a second. It is an average over the last few messages, and it disappears when the type stops arriving while the stream goes on.
**Base position** shows the latitude and longitude of the stream's base station once the stream has sent them (RTCM message 1005 or 1006), and **Base distance from GCS** how far the base station is from the [ground station position](gnss_receiver.md#gcs_position) when that is known. Both disappear when vehicles receive another stream or the stream stops for more than 5 seconds.
The toolbar GNSS indicator's drop-down shows the distance without the coordinates.
The toolbar GNSS indicator also offers **Connect** once an NTRIP server is configured.

## NTRIP Connection

Shows the NTRIP connection state with message and byte counters, the age of the last correction, and a Connect/Disconnect button.

## NTRIP Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — connect with TLS 1.2 or later
- **Accept self-signed TLS certificates** — accept a caster whose certificate is self-signed. The first successful connection records that certificate; a later connection that presents a different one fails, reporting that the caster's certificate changed. Turning the option off forgets the recorded certificate. Certificates signed by a trusted authority are never recorded.

## NTRIP Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

A caster answers with its source table when it does not have the requested mountpoint. Before any corrections have arrived, QGroundControl reports that and does not retry, because some public casters temporarily ban clients that keep requesting a missing mountpoint. Once the mountpoint has delivered corrections, the same answer means it went offline, and QGroundControl keeps reconnecting.

## GGA Position Reporting

Network RTK (VRS) casters need the rover position. Select the position QGC reports and how often.

## NTRIP Message Filter

- **RTCM Message Filter** — whitelist of RTCM message types to forward from NTRIP (empty = forward all)

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM corrections over UDP, for example from another application. UDP input is used alongside NTRIP and a local base station, and incoming data is always checked to be valid RTCM.
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)

## UDP Forwarding

- **Forward corrections over UDP** — forward the stream selected for vehicles to another device or application
- **UDP output address** — destination IP for forwarded data
- **UDP output port** — destination port for forwarded data

When forwarded data cannot be sent, **Corrections Status** names the destination and the reason the operating system gives, such as an unreachable network, until a send succeeds again.

## Correction Routing

- **Vehicle correction source** — _Highest priority_ sends one fresh stream to vehicles, preferring the local base station, then NTRIP, then UDP, and switches when the preferred stream stops. Selecting a source category uses only that category, without falling back to another. When several UDP senders deliver, QGC stays on the first fresh one.
- **Send corrections to** — _All vehicles_ (the default) sends the selected stream to every connected vehicle. _Active vehicle only_ sends it to the active vehicle alone: the corrections follow a change of the active vehicle, and no vehicle receives them while none is active. Corrections go out on the active vehicle's link, so vehicles that share that link, such as several on one telemetry radio, receive them as well.

Vehicles and UDP forwarding always receive the same stream, and UDP forwarding continues whichever vehicles are chosen.
