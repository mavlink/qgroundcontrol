# RTK Corrections

Configure where RTK corrections come from and how they reach vehicles: an NTRIP caster, the [GNSS receiver](gnss_receiver.md) as a base station, or RTCM received over UDP.

The page follows the work from top to bottom: which source is in use; NTRIP as it is filled in, the caster, the mountpoint and **Connect**; the saved casters and the options of the connection; then what is set less often, the position reported to a caster, UDP input and forwarding, and how corrections are routed to vehicles.

## What do you want to do? {#tasks}

### Use an NTRIP caster {#task_ntrip}

1. Open **RTK Corrections**. While no source is on, [Corrections Status](#corrections-status) reads _None_ and says what the sources are.
2. Under [NTRIP Caster](#ntrip-caster), enter the **Host address** and **Port** of the caster, and the **Username** and **Password** of your account. Turn on **Use TLS encryption** when the caster asks for it.
3. Under [NTRIP Mountpoint](#ntrip-mountpoint), select **Browse** and choose a mountpoint from the list, nearest first, or enter its name.
4. Under [NTRIP Connection](#ntrip-connection), select **Connect**. The line beside the button says what the connection is doing, and why it failed.
5. [Corrections Status](#corrections-status) names _NTRIP_ as the source once corrections arrive, and **Vehicle fix** shows what each vehicle makes of them.
6. For a network RTK (VRS) mountpoint, which sends nothing until it is told a position, see [GGA Position Reporting](#gga-position-reporting).

To change between casters without typing them again, keep each under a name in [Saved Casters](#saved-casters).
Once a caster is set up, the drop-down of the toolbar's GNSS indicator has its **Connect** and **Disconnect**, so the page is not needed in flight.

### Use a base station of your own

A GNSS receiver at the ground station is set up in the [GNSS Receiver](gnss_receiver.md) settings: as an [RTK base station with a survey](gnss_receiver.md#task_survey) or [from a saved position](gnss_receiver.md#task_saved_position), or as a [base station that is already set up](gnss_receiver.md#task_existing_base).
Its corrections need nothing on this page: with **Vehicle correction source** on _Highest priority_ they are preferred to NTRIP and UDP.

## Corrections Status

Shows the correction stream currently sent to vehicles and its data rate, whether it comes from the GNSS receiver, NTRIP, or UDP.
While no source is on, **Source** reads _None_ and a line below says what the sources are; _Waiting_ means that a source is on and no stream has arrived yet.
**Message Types** lists the RTCM message types of that stream and how many messages of each have arrived, such as `1077 × 120, every 1 s`. The counts start again when vehicles receive another stream, or when the stream resumes after it stopped for more than 5 seconds.
Once a second message of a type has arrived, its entry also says how often the type arrives: in whole seconds (`every 10 s`), or in messages per second (`5 Hz`) from two a second. It is an average over the last few messages, and it disappears when the type stops arriving while the stream goes on.
**Constellations** lists the satellite systems the stream has had observations of in the last 30 seconds, each with the number of satellites in its latest observations, such as `GPS 12, GLONASS 8, Galileo 9`. The number comes from the stream's MSM messages (1071 to 1137); a system that the stream describes with the older messages 1001 to 1004 or 1009 to 1012 is listed without one.
A warning appears below once a stream has run for 30 seconds and a vehicle cannot get an RTK fix from it: when it carries no satellite observations, such as a stream of station messages only or of other corrections than observations, or when it has not sent the position of its base station (message 1005 or 1006). Another warning says that the stream has GLONASS observations but not their code-phase biases (message 1230), without which a receiver of another make than the base station may not use GLONASS for an RTK fixed fix. These describe what is sent to vehicles, so a message the [RTCM message filter](#correction-routing) leaves out counts as missing.
**Base position** shows the latitude and longitude of the stream's base station once the stream has sent them (RTCM message 1005 or 1006), and **Base to ground station** how far the base station is from the [ground station position](gnss_receiver.md#gcs_position) when that is known. The distance is shown in the selected [distance unit](general.md#units): meters or feet, and kilometers or miles from one of those on. Both disappear when vehicles receive another stream or the stream stops for more than 5 seconds.
The toolbar GNSS indicator's drop-down shows the distance without the coordinates.
While the base station's position is known, the map of the [Fly View](../fly_view/fly_view.md) marks it with a **B** and draws a line from it to the active vehicle. On the line is the RTK baseline length that vehicle reports, while it reports one. The 3D map draws the line along the ground.
The toolbar GNSS indicator's drop-down also offers **Connect** once an NTRIP caster is set up.

### What the vehicles make of the corrections {#vehicle_fix}

**Queued to vehicles** counts the corrections handed to vehicle links since QGroundControl started, from whichever source they came. It counts what was queued for sending, not what a vehicle received.
**Vehicle fix** shows the fix type that a connected vehicle reports for its first GPS; with several vehicles each row names its vehicle, such as **Vehicle 2 fix**. A vehicle that can use the corrections changes to an RTK float fix and then to an RTK fixed fix. Below it are the **RTK Baseline** and the **RTK Correction Rate** of that GPS while the vehicle reports them (MAVLink `GPS_RTK`); not every autopilot does, and MAVLink has no age of the corrections to show.
The drop-down of the toolbar GNSS indicator leaves these two out for the active vehicle, whose **Vehicle GPS Status** there has them.

The label beside the toolbar GNSS indicator's icon follows the active vehicle too: **RTK** while it reports an RTK fixed fix, **Float** for an RTK float fix and **GNSS** otherwise. The label is orange while a correction source is on and the vehicle has no RTK fix, or no stream arrives. While no vehicle reports its GPS, the label says whether a correction source is on.

When the stream stops while a connected vehicle had an RTK fix from it, QGroundControl shows a vehicle alert below the toolbar, which needs no answer and closes by itself, and speaks it: 10 seconds after the stream stopped being fresh, and once for each interruption. When the stream is back, QGroundControl says so without showing anything. A vehicle counts when the corrections are [sent to it](#correction-routing) and it reported an RTK fix while they arrived.

## NTRIP Caster {#ntrip-caster}

- **Host address** — NTRIP caster hostname or IP address. Enter an IPv6 address without brackets, as `fe80::1`; the status shows it with them, as in `[fe80::1]:2101`.
- **Port** — NTRIP caster port (default: 2101)
- **Username** — NTRIP account username
- **Password** — NTRIP account password (with show/hide toggle)
- **Use TLS encryption** — connect with TLS 1.2 or later
- **Accept self-signed TLS certificates** — accept a caster whose certificate is self-signed. The first successful connection records that certificate; a later connection that presents a different one fails, reporting that the caster's certificate changed. Turning the option off forgets the recorded certificate. Certificates signed by a trusted authority are never recorded.

These settings and the mountpoint are locked while the connection is on.

QGroundControl asks a caster in the form NTRIP version 2 defines. A caster that speaks only version 1, such as RTKLIB's `str2str` up to 2.4.3 b33, closes such a request without answering it. QGroundControl then repeats the request once in the version 1 form, for the corrections and for the mountpoint list, with or without TLS. Once the caster has accepted that form, QGroundControl uses it first for that caster, also after a restart: it keeps the 16 casters that last accepted it. It goes back to the version 2 form for a caster that does not accept a version 1 request, whether the caster refuses it, for example for a wrong password, or closes it without answering.

## NTRIP Mountpoint

- **Mountpoint** — the NTRIP mountpoint to connect to. It is required: without one, NTRIP reports an error instead of connecting
- **Browse** — fetch the caster's mountpoints and list them. As far as the caster gives them, a row shows the mountpoint's description, format, navigation systems, carrier, country and bitrate, then what it asks for and the RTCM messages it sends with their intervals in seconds, such as `1005(10),1077(1)`. The carrier reads `L1` or `L1+L2`, or `no carrier phase` for corrections that give a differential fix at best, not an RTK one. **Needs your position** marks a mountpoint that sends corrections only for a position it is told ([GGA Position Reporting](#gga-position-reporting)), and **Needs a login** one that asks for a username and password. A mountpoint is listed when the caster names it, even without any of these; a name with a control character in it could not be requested and is left out. **Browse** does not use the **Mountpoint** field, so it also works while that field holds a name the caster does not have. When the caster's list names no mountpoint, QGroundControl says so. A list without its closing `ENDSOURCETABLE` line, as RTKLIB's `str2str` sends when its table file has none, is shown once the caster's answer is complete; one that is cut off before that is reported as an error.

The list is ordered by distance, nearest first, and shows each distance in the selected [distance unit](general.md#units), as **Base to ground station** does. The distances are measured from the active vehicle or, while no vehicle is connected or it has no position yet, from the [ground station position](gnss_receiver.md#gcs_position), as it is when **Browse** is pressed. Without either position the list keeps the caster's order and shows no distances, and a mountpoint the caster gives no coordinates for comes last.

**Filter by name** narrows the list to the mountpoints whose name or description contains the text entered, in upper or lower case, and says how many of the caster's mountpoints that leaves.

A row says so when the caster lists the mountpoint with something QGroundControl cannot use: a format other than RTCM 3, or Digest authentication. The mountpoint stays in the list and can still be selected, because a caster's list is not always right.
QGroundControl reports its position to the mountpoint it connects to whether or not the caster lists it as needing one.

QGroundControl receives RTCM 3 corrections only. A mountpoint that sends another format, such as SPARTN, is disconnected after 30 seconds of data without a valid RTCM message. Before any corrections have arrived, QGroundControl reports that and does not retry. Once the mountpoint has delivered corrections, it keeps reconnecting.

A caster answers with its source table, or with HTTP status 404 or 410, when it does not have the requested mountpoint. Before any corrections have arrived, QGroundControl reports that and does not retry, because some public casters temporarily ban clients that keep requesting a missing mountpoint. Once the mountpoint has delivered corrections, the same answer means it went offline, and QGroundControl keeps reconnecting.

A caster that answers with an HTTP redirect is not followed. QGroundControl reports the address the caster names, when it names one, and waits for you to change the caster settings and connect again.

## NTRIP Connection

The connection comes after what it connects to: the caster and the mountpoint above.
Shows the NTRIP connection state with message and byte counters, the age of the last correction, and a Connect/Disconnect button.
While a connection is being opened or retried, the button stops it. After an error it reads **Retry**, beside a **Disconnect** button that turns the connection off; **Connect** and **Retry** need a host address.
A double click or double tap on the button counts once, so it does not stop the connection it has just started.
The counters count everything the caster sends, including messages the [RTCM message filter](#correction-routing) keeps from vehicles.

QGroundControl reconnects when a stream that has delivered data sends nothing for 30 seconds. A stream that has not delivered anything yet has longer: it waits without a limit while there is no position to report to the caster, and 2 minutes once one has been reported ([GGA Position Reporting](#gga-position-reporting)).

## Saved Casters

Keeps casters under a name, so that changing between them does not mean typing them again.

- **Save Caster** — saves the caster and mountpoint settings above under the name entered: host address, port, username, password, the two TLS options and the mountpoint. A name that is already saved is replaced. Up to 20 casters are kept.
- **Use Caster** — copies the selected caster into the caster and mountpoint settings. The connection always uses those settings, so a saved caster has no effect until it is used. Like the settings it writes, it is available only while the connection is off.
- **Delete Caster** — removes the selected caster from the list. The caster and mountpoint settings stay as they are.

A saved caster keeps its password. It is stored with the application's settings, as the **Password** above is, and is not encrypted there: anyone who can read the settings file can read it.

A caster that accepts self-signed certificates is saved with the certificate recorded for it, if one was. **Use Caster** never carries the certificate recorded for one caster over to another: when the recorded certificate belongs to another host and port, it is replaced by the one saved with the caster, or forgotten when none was. A certificate recorded after a caster was saved is kept with it only when the caster is saved again.

### Falling back to saved casters {#failover}

**Fall back to saved casters** is off by default. With it on, QGroundControl goes on to the saved casters when the caster in use cannot be reached or stops serving, instead of retrying that one alone:

- It starts with the caster that is entered, then takes the saved casters in the order of the list, and after the last one the entered caster again.
- It leaves a caster after three failed attempts in a row without any corrections between them. The pauses between attempts are the same as without this option, whichever caster the next attempt goes to, and after 100 attempts without corrections it gives up as before.
- A failure that is not retried still ends the connection with its error when the caster is the one entered: a refused login, a mountpoint the caster does not have, or a certificate that is not accepted. These are yours to correct, and another caster would only hide them. A saved caster that fails in such a way is left out until the connection is started again, and the next one is tried.
- It does not return to an earlier caster while the one in use delivers corrections. **Connect**, **Retry** and any change of the caster or mountpoint settings start again with the entered caster. So does a change of the saved casters or of this option while a saved caster is in use.
- A saved caster takes no part when it is the same as the entered one, when it has no mountpoint, or when it accepts self-signed certificates and none was recorded for it when it was saved: its certificate could not be compared with one trusted before.

While a saved caster is in use, or is the next to be tried, the connection status ends with its name, such as **Connected (saved caster Backup)**, and **Mountpoint** under [NTRIP Connection](#ntrip-connection) shows that caster's mountpoint.

## NTRIP Connection Options {#connection-options}

- **Connect only with a vehicle** — off by default. With it on, a connection that is turned on waits until a vehicle is connected, and the connection status reads **Waiting for a vehicle**; the button stops the wait. The connection starts as soon as a vehicle connects. When the last vehicle is gone it is kept for 10 seconds, so that a link that drops and comes back does not restart the stream, and then ended until the next vehicle. The vehicle QGroundControl keeps for planning without a connection does not count.
- **Data limit** — how many megabytes the connection may receive; 0, the default, sets no limit. Once that much has arrived from the caster the connection ends, and its status reads **Stopped at the data limit of 50 MB** for a limit of 50. It stays ended until **Retry**, or until the limit is raised or set to 0: changing any other setting does not start it. A limit set below what has already arrived ends the connection as well.
  The limit counts everything the caster sends over the correction connection, with the response around the stream and data that holds no valid correction, so it is reached a little before **Data Received** shows the same figure. The mountpoint list that **Browse** fetches is not counted. A megabyte is 1024 × 1024 bytes, as in **Data Received**.
  The count runs from **Connect** or **Retry**, through reconnects, [saved casters](#failover) and waits for a vehicle, and starts again when the connection is turned off and on. It is not kept when QGroundControl is closed. The limit is on one run of the connection, not on a month of a data plan, which QGroundControl knows nothing of: a connection that is turned on connects again after a restart with the whole limit to use.

## GGA Position Reporting

Network RTK (VRS) casters need the rover position. Select the position QGC reports and how often.
**GGA Position Source** is _Auto_, which selects the best position there is, or one of _Vehicle GPS_, _Vehicle EKF_, _GNSS receiver_ and _Ground station position_.

A network RTK mountpoint sends nothing until it has received a position. While none is available, for example before the vehicle has a GPS fix, QGroundControl stays connected and the connection status reads **Connected, waiting for a position to report**.

Once it has a position, the caster works out corrections for it before it sends any, which can take a minute or more. Until the first corrections arrive the connection status reads **Connected, waiting for corrections**, and QGroundControl gives the caster 2 minutes from the first position reported to send its first data before it reconnects. From the first data on, the [30-second limit](#ntrip-connection) on a silent stream applies.

The report carries the selected position's height above mean sea level and the geoid separation that goes with it.
When the position comes with its height above the WGS84 ellipsoid as well, the separation is the receiver's own, the difference between the two heights. The GNSS receiver's position usually has both, and so has the vehicle GPS position when the vehicle reports the ellipsoid height (`alt_ellipsoid` in `GPS_RAW_INT`), as PX4 and ArduPilot do for most receivers.
Otherwise the separation comes from the EGM96 geoid model built into QGroundControl.
A height above the WGS84 ellipsoid alone, as the position of an Android device or of a base station has, is converted with that model.
A height of unknown reference, as the position of a Linux device has, is reported as it is, and a position without a height is reported with height 0, both without a geoid separation. A caster places the rover by its latitude and longitude.

## UDP RTCM Input

- **Enable UDP RTCM input** — receive RTCM corrections over UDP, for example from another application. UDP input is used alongside NTRIP and the GNSS receiver, and incoming data is always checked to be valid RTCM.
- **UDP RTCM input port** — port to listen on (default: 13320, range: 1024–65535)

## UDP Forwarding

- **Forward corrections over UDP** — forward the stream selected for vehicles to another device or application
- **UDP output address** — destination IP for forwarded data
- **UDP output port** — destination port for forwarded data

When forwarded data cannot be sent, **Corrections Status** names the destination and the reason the operating system gives, such as an unreachable network, until a send succeeds again.

## Correction Routing

- **Vehicle correction source** — _Highest priority_ sends one fresh stream to vehicles, preferring the GNSS receiver, then NTRIP, then UDP, and switches when the preferred stream stops. Selecting a source category uses only that category, without falling back to another. When several UDP senders deliver, QGC stays on the first fresh one.
- **Send corrections to** — _All vehicles_ (the default) sends the selected stream to every connected vehicle. _Active vehicle only_ sends it to the active vehicle alone: the corrections follow a change of the active vehicle, and no vehicle receives them while none is active. Corrections go out on the active vehicle's link, so vehicles that share that link, such as several on one telemetry radio, receive them as well.
- **RTCM Message Filter** — comma-separated RTCM message IDs (1 to 4095), such as `1005,1077,1087`. Only these messages of the selected stream are used, whichever source it comes from: NTRIP, the GNSS receiver or UDP. Empty uses every message.

Messages the filter leaves out are not sent to vehicles, not forwarded over UDP and not counted under **Message Types**. A stream of which the filter leaves nothing counts as delivering no corrections, so with _Highest priority_ the next source is used instead.
While that leaves no stream to send, **Corrections Status** says so and lists the message IDs that arrive, for example when a filter written for an NTRIP stream meets a base station of your own that sends other messages.
An entry that is not a message ID stops all corrections until it is corrected, and **Corrections Status** names the entry: a mistyped filter never sends every message.

A vehicle receives no corrections while it is connected through a link marked **High Latency** in its [link settings](comm_links.md), such as a satellite modem: a correction stream is far more data than such a link carries. Corrections reach the vehicle again once a regular link takes over.

Vehicles and UDP forwarding always receive the same stream, and UDP forwarding continues whichever vehicles are chosen.

**Log corrections to a file** writes the corrections to a file as they are sent, to send with a problem report or to post-process the base station's observations.
It is off each time QGroundControl starts. Each time it is turned on it starts a new file in the `GPSCorrections` folder of the [Application Load/Save Path](general.md#load_save_path), named with the UTC time it started, such as `20260102T030405.006Z.rtcm3`.
The file's path is shown below the switch while the log is written.
The file holds the RTCM 3 messages exactly as vehicles and UDP forwarding receive them, one after the other, which is the raw RTCM 3 format other tools read: only the stream selected for vehicles, without the messages the RTCM message filter leaves out.
It therefore includes the base station location, so share it only where that is acceptable.
A log stops when its file reaches 256 MB or cannot be written: the switch turns off, the path is no longer shown, and a message below the switch says why until the log is turned on again. A log that cannot start says why in the same place. The corrections continue either way.
