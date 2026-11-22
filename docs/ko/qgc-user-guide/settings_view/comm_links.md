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

## Android Serial

On Android, **Use built-in serial ports** adds the serial ports of the device itself (`/dev/tty*`) to the serial port list, alongside USB serial devices.
Use it on a ground controller whose radio or serial connector is wired to one of these ports.
It needs a device that lets apps open them.

The list holds every such port that the device lets QGroundControl open.
Some of them can belong to the device's own hardware, such as its Bluetooth radio, and connecting to one of those can disturb it.
Use the port that the maker of the device names for the vehicle link.
