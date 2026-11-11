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
The counters count everything the caster sends, including messages the [RTCM message filter](#correction-routing) keeps from vehicles.

## NTRIP Server

- **Host address** — NTRIP caster hostname or IP
- **Server port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — connect with TLS 1.2 or later
- **Accept self-signed TLS certificates** — accept a caster whose certificate is self-signed. The first successful connection records that certificate; a later connection that presents a different one fails, reporting that the caster's certificate changed. Turning the option off forgets the recorded certificate. Certificates signed by a trusted authority are never recorded.

## NTRIP Mountpoint

- **Mount Point** — the NTRIP mountpoint to connect to. It is required: without one, NTRIP reports an error instead of connecting
- **Browse** — fetch available mountpoints from the server and display them in a list with format, navigation system, country, bitrate, and distance information

QGroundControl receives RTCM 3 corrections only. A mountpoint that sends another format, such as SPARTN, is disconnected after 30 seconds of data without a valid RTCM message. Before any corrections have arrived, QGroundControl reports that and does not retry. Once the mountpoint has delivered corrections, it keeps reconnecting.

A caster answers with its source table, or with HTTP status 404 or 410, when it does not have the requested mountpoint. Before any corrections have arrived, QGroundControl reports that and does not retry, because some public casters temporarily ban clients that keep requesting a missing mountpoint. Once the mountpoint has delivered corrections, the same answer means it went offline, and QGroundControl keeps reconnecting.

A caster that answers with an HTTP redirect is not followed. QGroundControl reports the address the caster names, when it names one, and waits for you to change the server settings and connect again.

## GGA Position Reporting

Network RTK (VRS) casters need the rover position. Select the position QGC reports and how often.

A network RTK mountpoint sends nothing until it has received a position. While none is available, for example before the vehicle has a GPS fix, QGroundControl stays connected and the connection status reads **Connected, waiting for a position to report**. The 30-second limit on a silent stream starts with the first position reported or the first data received.

The report carries the altitude of the selected position above mean sea level. A position whose height is not known to be above mean sea level, as for the position of an Android, Linux or Windows device or of a base station, and a position without a height are reported with the altitude left empty. A caster places the rover by its latitude and longitude.

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
- **RTCM Message Filter** — comma-separated RTCM message IDs (1 to 4095), such as `1005,1077,1087`. Only these messages of the selected stream are used, whichever source it comes from: NTRIP, the local base station or UDP. Empty uses every message.

Messages the filter leaves out are not sent to vehicles, not forwarded over UDP and not counted under **Message Types**. A stream of which the filter leaves nothing counts as delivering no corrections, so with _Highest priority_ the next source is used instead.
While that leaves no stream to send, **Corrections Status** says so and lists the message IDs that arrive, for example when a filter written for an NTRIP stream meets a local base station that sends other messages.
An entry that is not a message ID stops all corrections until it is corrected, and **Corrections Status** names the entry: a mistyped filter never sends every message.

A vehicle receives no corrections while it is connected through a link marked **High Latency** in its [link settings](comm_links.md), such as a satellite modem: a correction stream is far more data than such a link carries. Corrections reach the vehicle again once a regular link takes over.

Vehicles and UDP forwarding always receive the same stream, and UDP forwarding continues whichever vehicles are chosen.

**Log corrections to a file** writes the corrections to a file as they are sent, to send with a problem report or to post-process the base station's observations.
It is off each time QGroundControl starts. Each time it is turned on it starts a new file in the `GPSCorrections` folder of the [Application Load/Save Path](general.md#load_save_path), named with the UTC time it started, such as `20260102T030405.006Z.rtcm3`.
The file's path is shown below the switch while the log is written.
The file holds the RTCM 3 messages exactly as vehicles and UDP forwarding receive them, one after the other, which is the raw RTCM 3 format other tools read: only the stream selected for vehicles, without the messages the RTCM message filter leaves out.
It therefore includes the base station location, so share it only where that is acceptable.
A log stops when its file reaches 256 MB or cannot be written: the switch turns off, the application log reports it and the path is no longer shown. The corrections continue either way.
