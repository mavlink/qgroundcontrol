# QGC Simulation Tools

Tools for testing QGroundControl without physical hardware.

## Quick Start

| Tool | Use Case | Setup |
| --- | --- | --- |
| `mock_vehicle.py` | UI testing, quick checks | `python tools/setup/install_python.py dev` |
| `run_arducopter_sitl.py` | Full simulation, mission testing | Docker required |
| `rc_transmitter.py` | RC transmitter for PX4 SITL: flight modes, switches, RC loss | `python tools/setup/install_python.py dev` |

## Mock Vehicle (Lightweight)

A minimal MAVLink simulator for UI testing. Does not simulate flight dynamics.

```bash
# Install
python tools/setup/install_python.py dev

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
| --- | --- | --- |
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
./run_arducopter_sitl.py

# With simulated network latency (Herelink-like)
./run_arducopter_sitl.py --with-latency

# Connect QGC to: tcp://localhost:5760
```

**Docker Commands:**

```bash
docker logs -f arducopter-sitl   # View logs
docker stop arducopter-sitl      # Stop simulation
docker rm arducopter-sitl        # Remove container
```

**Requirements:** Docker

Mock Vehicle auto-connects over UDP 14550. For SITL, add a TCP comm link in QGC
(**Application Settings → Comm Links → Add**, Host `localhost`, Port `5760`).

## Virtual RC Transmitter (PX4 SITL)

Simulates an RC transmitter for PX4 SITL, which has no RC receiver. A web page has a slider and
1000/1500/2000 buttons for each of the 18 channels. The script streams the PWM values as
`RC_CHANNELS_OVERRIDE`, which PX4 treats as receiver input, so mode channels, mode buttons and
switches behave as with a real transmitter. QGC stays connected on its own link at the same time.

```bash
# SITL on this machine: uses the PX4 offboard link (UDP 14540), not QGC's 14550
./rc_transmitter.py

# SITL in a VM or on another machine: connect to its offboard link port
./rc_transmitter.py --connect udpout:10.211.55.7:14580

# Then open http://localhost:8080
```

PX4 SITL only reads MAVLink joystick input by default. Set it to use RC, and map the sticks so
the mode channel and switches can change modes:

```text
param set COM_RC_IN_MODE 0
param set RC_CHAN_CNT 8
param set RC_MAP_ROLL 1
param set RC_MAP_PITCH 2
param set RC_MAP_THROTTLE 3
param set RC_MAP_YAW 4
```

**Stopped** pauses sending, to test the RC loss failsafe.

**Options:**

| Option | Default | Description |
| --- | --- | --- |
| `--connect` | `udpin:0.0.0.0:14540` | pymavlink connection to the vehicle |
| `--port` | 8080 | Web page port |
| `--rate` | 20 | Send rate (Hz). PX4 treats RC as lost after `COM_RC_LOSS_T` (0.5 s) without it |

## Comparison

| Feature | mock_vehicle.py | ArduCopter SITL |
| --- | --- | --- |
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

The SITL launcher binds TCP port 5760 to localhost and waits for the guest listener without
consuming its first client connection. It replaces only containers carrying its ownership label.
Remove a container created by the previous shell launcher explicitly before using the same name.
