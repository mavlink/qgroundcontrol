# Comm Links

Configure how QGroundControl connects to vehicles.

## Auto-Connect

By default, QGC auto-detects and connects to common devices:

- **Pixhawk** — auto-connect Pixhawk flight controllers via USB
- **SiK Radio** — auto-connect SiK telemetry radios
- **LibrePilot** — auto-connect LibrePilot controllers
- **UDP** — auto-connect via UDP broadcast
- **RTK GPS** — auto-connect RTK GPS base stations

## GCS Position

Choose where the ground station position (used for the map, Follow Me, NTRIP GGA, and Remote ID)
comes from:

- **Automatic** — the connected [RTK receiver](ntrip_rtk.md#rtk-gps-receiver), then the NMEA GPS,
  then this device's positioning. QGC switches to the next source when the preferred one loses its
  fix, and switches back after the preferred source has been healthy for five seconds.
- **RTK receiver**, **NMEA GPS**, or **This device** — use only that source.

The section also shows the source in use, its status, and the current position.

## NMEA GPS

Configure an external NMEA GPS device to provide GCS position:

- **Device** — Disabled, UDP, Serial port, or TCP client
- **Baudrate** — serial baud rate (with custom baud option)
- **UDP port** — port for UDP NMEA input (default: 14401)
- **TCP server host** and **port** — NMEA server for the TCP client

## Link Management

Manually create and manage communication links when auto-connect is insufficient:

- **Add New Link** — create a link with a name, type, and connection-specific settings:
  - **Serial** — port, baud rate, flow control
  - **TCP** — host address, port
  - **UDP** — local port, remote hosts
  - **Bluetooth** — device selection
- **Auto Connect on Start** — automatically connect this link when QGC starts
- **High Latency** — mark the link as high-latency (reduces message rate)

Existing links can be edited, deleted, connected, or disconnected from the link list.
