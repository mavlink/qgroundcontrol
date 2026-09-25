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

- **Automatic** — the connected [GNSS receiver](ntrip_rtk.md#gnss-receiver), then this device's
  positioning. QGC switches to this device when the receiver loses its fix, and switches back after
  the receiver has been healthy for five seconds.
- **GNSS receiver** or **This device** — use only that source.

The section also shows the source in use, its status, and the current position.

External NMEA GPS devices are configured as a [GNSS receiver](ntrip_rtk.md#gnss-receiver) with the
_Position only_ role.

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
