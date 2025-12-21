# QGC Simulation Tools

Tools for testing QGroundControl without physical hardware.

## Quick Start

| Tool | Use Case | Setup |
|------|----------|-------|
| `mock_vehicle.py` | UI testing, quick checks | `pip install pymavlink` |
| `run-arducopter-sitl.sh` | Full simulation, mission testing | Docker required |

## Mock Vehicle (Lightweight)

A minimal MAVLink simulator for UI testing. Does not simulate flight dynamics.

```bash
# Install
pip install pymavlink

# Run (QGC connects to UDP 14550)
./mock_vehicle.py

# Multiple vehicles
./mock_vehicle.py --system-id 1 --port 14550 &
./mock_vehicle.py --system-id 2 --port 14551 &

# TCP mode (SITL-style)
./mock_vehicle.py --tcp --port 5760
```

**Features:**
- Heartbeat, GPS, attitude, battery telemetry
- Arm/disarm commands
- Mode changes
- Basic parameter support

**Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `--host` | 127.0.0.1 | Host address |
| `--port` | 14550 | Port number |
| `--tcp` | off | Use TCP instead of UDP |
| `--system-id` | 1 | MAVLink system ID |
| `--rate` | 10 | Telemetry rate (Hz) |
| `--sitl` | off | SITL-compatible mode (TCP:5760) |

**Limitations:** No flight dynamics, no mission execution, limited MAVLink support.

## ArduCopter SITL (Full Simulation)

Full ArduPilot simulation via Docker. Supports missions, geofences, all commands.

```bash
# Run (builds image on first run, ~10-15 min)
./run-arducopter-sitl.sh

# With simulated network latency (Herelink-like)
./run-arducopter-sitl.sh --with-latency

# Connect QGC to: tcp://localhost:5760
```

**Docker Commands:**
```bash
docker logs -f arducopter-sitl   # View logs
docker stop arducopter-sitl      # Stop simulation
docker rm arducopter-sitl        # Remove container
```

**Requirements:** Docker

## Connecting QGC

### Mock Vehicle (UDP)
1. Start `./mock_vehicle.py`
2. QGC auto-connects to UDP 14550

### ArduCopter SITL (TCP)
1. Start `./run-arducopter-sitl.sh`
2. In QGC: **Application Settings → Comm Links → Add**
3. Type: TCP, Host: localhost, Port: 5760

## Comparison

| Feature | mock_vehicle.py | ArduCopter SITL |
|---------|-----------------|-----------------|
| Setup time | Seconds | 10-15 min (first run) |
| Dependencies | pymavlink | Docker |
| Flight dynamics | ❌ | ✓ |
| Mission execution | ❌ | ✓ |
| Geofence support | ❌ | ✓ |
| All MAVLink commands | ❌ | ✓ |
| Resource usage | Low | Medium |
| Best for | UI testing | Integration testing |

## Other Simulators

For more advanced simulation:
- [ArduPilot SITL](https://ardupilot.org/dev/docs/sitl-simulator-software-in-the-loop.html) - Native install
- [PX4 SITL](https://docs.px4.io/main/en/simulation/) - PX4 simulation
- [Gazebo](https://gazebosim.org/) - 3D physics simulation
