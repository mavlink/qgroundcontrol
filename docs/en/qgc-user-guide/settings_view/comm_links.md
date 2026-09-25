# Comm Links

Configure how QGroundControl connects to vehicles.

## Auto-Connect

By default, QGC auto-detects and connects to common devices:

- **Pixhawk** — auto-connect Pixhawk flight controllers via USB
- **SiK Radio** — auto-connect SiK telemetry radios
- **LibrePilot** — auto-connect LibrePilot controllers
- **UDP** — auto-connect via UDP broadcast
- **RTK GPS** — auto-connect RTK GPS base stations

## NMEA GPS

Configure an external NMEA GPS device to provide GCS position (used for RTK and Remote ID):

- **Device** — Disabled, UDP, or Serial port
- **Baudrate** — serial baud rate (with custom baud option)
- **UDP port** — port for UDP NMEA input (default: 14401)

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
