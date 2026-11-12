# Comm Links

Configure how QGroundControl connects to vehicles.

## Auto-Connect

By default, QGC auto-detects and connects to common devices:

- **Pixhawk** — auto-connect Pixhawk flight controllers via USB
- **SiK Radio** — auto-connect SiK telemetry radios
- **LibrePilot** — auto-connect LibrePilot controllers
- **UDP** — auto-connect via UDP broadcast

GNSS receivers, including NMEA devices that provide the GCS position, are connected in the [GNSS Receiver](gnss_receiver.md) settings.

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

A link cannot be connected while a [flight data replay](../fly_view/replay_flight_data.md) is playing, or while [Firmware](../setup_view/firmware.md) setup is looking for a board or flashing it.
**Connect** then shows a message that names the reason, and auto-connect waits until it has ended.
