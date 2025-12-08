# Mock MAVLink Vehicle

A simple MAVLink vehicle simulator for testing QGroundControl without actual hardware.

## Installation

```bash
pip install pymavlink
```

## Usage

```bash
# Default: UDP on localhost:14550 (connect QGC to UDP port 14550)
./mock_vehicle.py

# Custom port
./mock_vehicle.py --port 14551

# TCP connection (for SITL-style testing)
./mock_vehicle.py --tcp --port 5760

# SITL-compatible mode
./mock_vehicle.py --sitl

# Multiple vehicles
./mock_vehicle.py --system-id 1 --port 14550 &
./mock_vehicle.py --system-id 2 --port 14551 &
```

## Connecting QGC

1. Start the mock vehicle
2. In QGC, go to **Application Settings > Comm Links**
3. Add a new link:
   - **Type**: UDP
   - **Port**: 14550 (or your chosen port)
   - **Server**: Leave empty for localhost
4. Connect

## Features

- Heartbeat messages (1 Hz)
- GPS position (simulated in Zurich)
- Attitude and heading
- Battery status
- System status
- Arm/disarm commands
- Mode changes
- Basic parameter support

## Simulated Behavior

When armed:
- Vehicle slowly rotates (heading changes)
- Small position drift
- Battery slowly drains

## Command Line Options

| Option | Default | Description |
|--------|---------|-------------|
| `--host` | 127.0.0.1 | Host address |
| `--port` | 14550 | Port number |
| `--tcp` | off | Use TCP instead of UDP |
| `--system-id` | 1 | MAVLink system ID |
| `--rate` | 10 | Telemetry rate (Hz) |
| `--sitl` | off | SITL-compatible mode (TCP:5760) |

## Limitations

This is a minimal simulator for UI testing. It does not:
- Simulate flight dynamics
- Support mission execution
- Provide realistic sensor data
- Support all MAVLink commands

For full simulation, use:
- [ArduPilot SITL](https://ardupilot.org/dev/docs/sitl-simulator-software-in-the-loop.html)
- [PX4 SITL](https://docs.px4.io/main/en/simulation/)
