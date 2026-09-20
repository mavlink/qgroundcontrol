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

## Local Serial Receiver

Open the GPS indicator and expand **RTK GPS Settings** to connect a local receiver.
**Auto-connect known receivers** retains discovery for recognized receiver devices.
A generic CH340, FTDI, or other USB serial adapter does not identify the attached GNSS
receiver and is not automatically configured as Unicore or Quectel.

For **Unicore UM980/UM982**, **Quectel LG290P**, or **Passive RTCM/NMEA**:

1. Choose the specific receiver under **Receiver / settings**, not **All**.
2. Choose its **Serial device** and **Baud rate**. Connect only the intended receiver.
3. Select a supported base mode, then press **Connect**. Manual connections disable
   auto-connect. Use **Disconnect** before changing the receiver or its settings.

Settings are read on each connection. Reconnect the receiver to apply changes;
restarting QGroundControl is not required.

After a survey completes, **Save Current Base Position** copies its coordinates and
receiver-reported accuracy into the fixed-position settings. This is available while
the survey receiver remains connected and does not reconfigure it. Then disconnect,
select **Specify position**, and reconnect to use the saved position. Saving requires
valid coordinates and a known, finite accuracy; receiver-managed averaging without
accuracy telemetry cannot supply a fixed-position accuracy.

The serial port is reserved while the receiver is running, including shutdown.
Ports already used by a vehicle link or another GPS source cannot be taken over.
Connection and configuration failures appear in the GPS indicator.

### Base Modes

- **Specify position** uses latitude, longitude, and altitude above the WGS84
  ellipsoid, not mean sea level. Unicore and Quectel support this mode.
- **Survey-In** on Quectel LG290P counts accepted position observations at 1 Hz.
  **Accepted observation time** counts these observations in equivalent seconds;
  elapsed time can be longer. **Observation accuracy limit** filters individual
  positions and does not guarantee final base-position accuracy. Unicore does not
  support this mode.
- **Receiver-managed averaging** is a separate Unicore mode. **Maximum averaging
  time** accepts 1–3600 seconds. It is not accuracy-controlled survey-in and does not
  guarantee a position accuracy. The receiver's stored-coordinate reuse threshold
  is disabled; it is not an accuracy threshold. Quectel does not support this mode.

Changing receiver type does not silently replace an incompatible base mode.
Select a supported mode explicitly.

By default, Quectel role and base settings must match settings saved externally with
the manufacturer's tools. Mismatches are reported as errors; QGroundControl does not
silently save settings.

For an explicit manual connection, **Allow flash save and restart** permits
QGroundControl to apply and save requested role or base-setting changes, then restart
the receiver. The permission is off by default, applies only to that connection
attempt, and is cleared after every attempt. It is not saved as an application
setting and is never applied by auto-connect. Only changes requiring persistence
trigger a save; no factory reset is performed. **Saved changes can remain on the
receiver even if reconnecting fails.**

The Quectel receiver restarts on connection, including fixed-base mode. Survey-in
restarts with stored-coordinate reuse disabled. Corrections are forwarded only after
the receiver reports a valid base solution. The receiver's own firmware may
automatically store completed survey coordinates independently of this permission.

**Passive RTCM/NMEA** never sends receiver configuration commands. Configure the
receiver's output externally and select its existing serial baud rate. QGroundControl
forwards validated RTCM and displays available satellite observations without inferring
survey-in active/valid state or base-position accuracy from ordinary NMEA fixes.

These new receiver integrations have automated protocol tests but have not been
qualified on physical hardware. Verify receiver firmware compatibility and the
correction stream before operational use.
