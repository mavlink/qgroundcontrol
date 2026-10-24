# QGC Simulation Tools

Tools for testing QGroundControl without physical hardware. Run them from the repository root.

## Quick Start

| Tool | Use Case | Setup |
| --- | --- | --- |
| `mock_vehicle.py` | UI testing, quick checks | `python3 tools/setup/install_python.py simulation` (pymavlink), then activate `tools/.venv` |
| `run_arducopter_sitl.py` | Full simulation, mission testing | Docker, or an ArduPilot SITL build with `--native` |
| `fly_plan.py` | Fly a QGC `.plan` in ArduPilot or PX4 SITL and record the track and images | `python3 tools/setup/install_python.py simulation` (pymavlink), then activate `tools/.venv` |
| `rc_transmitter.py` | RC transmitter for PX4 SITL: flight modes, switches, RC loss | `python3 tools/setup/install_python.py simulation` (pymavlink), then activate `tools/.venv` |
| `mock_ntrip_caster.py` | NTRIP/RTK client testing | Python standard library only |

## Mock Vehicle (Lightweight)

A minimal MAVLink simulator for UI testing. Does not simulate flight dynamics.

```bash
# Run (QGC connects to UDP 14550)
python tools/simulation/mock_vehicle.py

# Multiple vehicles
python tools/simulation/mock_vehicle.py --system-id 1 --port 14550 &
python tools/simulation/mock_vehicle.py --system-id 2 --port 14551 &

# TCP mode (SITL-style): listens for QGC's TCP comm link
python tools/simulation/mock_vehicle.py --tcp --port 5760
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
| `--tcp` | off | Listen on TCP instead of sending UDP |
| `--system-id` | 1 | MAVLink system ID |
| `--rate` | 10 | Telemetry rate (Hz) |
| `--sitl` | off | SITL-compatible mode (listen on TCP 5760) |
| `--lat`, `--lon`, `--alt` | `MOCK_VEHICLE_LAT`/`_LON`/`_ALT`, else 47.3977, 8.5456, 100 m | Initial position |

**Limitations:** No flight dynamics, no mission execution, limited MAVLink support.

Mock Vehicle auto-connects over UDP 14550. In TCP mode, add a TCP comm link in QGC to the host and
port it listens on.

## ArduCopter SITL (Full Simulation)

Full ArduPilot simulation via Docker, or a [local build](#without-docker). Supports missions,
geofences, all commands.

```bash
# Run (builds image on first run, ~10-15 min)
python tools/simulation/run_arducopter_sitl.py

# With simulated network latency (Herelink-like)
python tools/simulation/run_arducopter_sitl.py --with-latency

# Connect QGC to: tcp://localhost:5760
```

`--name` sets the container name (default `arducopter-sitl`) and `--timeout` the seconds to wait
for the TCP listener (default 60). The launcher binds TCP port 5760 to localhost and waits for the
guest listener without consuming its first client connection. It replaces only containers carrying
its ownership label; remove a container created by the previous shell launcher explicitly before
using the same name.

The image is built from the third-party `radarku/ardupilot-sitl-docker` Dockerfile, pinned to a
reviewed commit (`BUILD_CONTEXT`). An existing `ardupilot-sitl-4.5.6` image is reused; remove it
with `docker rmi ardupilot-sitl-4.5.6` to rebuild after the pin changes.

Add a TCP comm link in QGC (**Application Settings → Comm Links → Add**, Host `localhost`,
Port `5760`).

**Docker Commands:**

```bash
docker logs -f arducopter-sitl   # View logs
docker stop arducopter-sitl      # Stop simulation
docker rm arducopter-sitl        # Remove container
```

### Without Docker

`--native` runs the SITL build in an ArduPilot checkout with the same vehicle, home position, and
parameters as the Docker image.

```bash
# Build ArduCopter SITL once
git clone --recurse-submodules https://github.com/ArduPilot/ardupilot.git ~/ardupilot
cd ~/ardupilot
Tools/environment_install/install-prereqs-ubuntu.sh -y
./waf configure --board sitl
./waf copter

# Run from the QGC repository root; Ctrl+C stops it
python tools/simulation/run_arducopter_sitl.py --native ~/ardupilot
```

Without root access, skip `install-prereqs-ubuntu.sh` and install the build's Python packages into
a virtual environment instead: `empy==3.3.4 future lxml pexpect ptyprocess dronecan`.

ArduPilot SITL exits when the process that started it does, so the launcher stays in the
foreground. Each run starts in a new temporary directory, which holds the log and the vehicle's
parameters. SITL listens on all interfaces: QGC connects on TCP 5760, and TCP 5762 and 5763 are
spare MAVLink ports for other tools such as `fly_plan.py`. `--with-latency` needs Docker.

## PX4 SITL (SIH)

PX4's simulation-in-hardware (SIH) model runs PX4 SITL with its own physics, without Gazebo.

```bash
git clone --recursive https://github.com/PX4/PX4-Autopilot.git ~/PX4-Autopilot
cd ~/PX4-Autopilot
make px4_sitl sihsim_quadx
```

QGC connects automatically over UDP 14550. Other tools, such as `fly_plan.py` and
`rc_transmitter.py`, use the offboard link on UDP 14540. Set `PX4_SIM_SPEED_FACTOR` to run faster
than real time, and `PX4_HOME_LAT`, `PX4_HOME_LON`, and `PX4_HOME_ALT` to start somewhere else,
such as a plan's planned home position:

```bash
PX4_SIM_SPEED_FACTOR=5 PX4_HOME_LAT=47.6335 PX4_HOME_LON=-122.0898 PX4_HOME_ALT=0 \
    make px4_sitl sihsim_quadx
```

## Fly a Plan

`fly_plan.py` checks mission generation against a real autopilot without driving the QGC user
interface. It uploads a saved `.plan`, flies it in AUTO, and records the flown track and the
position of every image the autopilot reports.

```bash
# ArduPilot SITL: a spare serial port, so QGC can stay connected on its own link
./fly_plan.py survey.plan --connect tcp:127.0.0.1:5762 --svg flight.svg

# PX4 SITL: the offboard link
./fly_plan.py survey.plan --connect udpin:0.0.0.0:14540 --svg flight.svg
```

The plan's simple items and the mission items QGC saves for surveys and corridor scans are uploaded
as QGC would upload them. Structure scans and landing patterns do not save their mission items, so
upload those plans from QGC. ArduPilot takes off in GUIDED to `--takeoff-alt` before switching to
AUTO, since it does not arm in AUTO by default; PX4 starts the mission on the ground. Afterwards the
vehicle returns to launch unless `--no-rtl` is given.

The JSON output has the uploaded mission, the track (position, altitude, heading and current
mission item), the images, and the vehicle's status messages. `--svg` plots the planned path
(dashed), the flown track, and the images. The script exits with 1 if the mission does not
complete within `--timeout`.

Images are only reported with a camera configured:

- ArduPilot: `CAM1_TYPE 1`, then reboot. Images are reported as `CAMERA_FEEDBACK`.
- PX4: `TRIG_MODE 4` and `TRIG_INTERFACE 3`, then restart or run `camera_trigger start`. Images
  are reported as `CAMERA_IMAGE_CAPTURED`.

| Option | Default | Description |
| --- | --- | --- |
| `--connect` | required | pymavlink connection to the vehicle |
| `--output` | `flight.json` | JSON output |
| `--svg` | off | Also plot the planned and flown paths |
| `--takeoff-alt` | 10 | ArduPilot takeoff altitude before AUTO (m) |
| `--timeout` | 1800 | Seconds to wait for the mission to complete |
| `--no-rtl` | off | Do not return to launch afterwards |

## Mock NTRIP Caster

Serves an NTRIP source table and streams RTCM3 1005/1077 frames with valid CRC-24Q, for testing
QGC's RTK corrections client without a real caster.

```bash
python tools/simulation/mock_ntrip_caster.py                 # Listen on 127.0.0.1:2101
python tools/simulation/mock_ntrip_caster.py --host 0.0.0.0  # Accept LAN connections
```

In QGC, open **Application Settings → RTK Corrections → NTRIP Server**, set host `127.0.0.1`, port
`2101`, and mount point `MOCK1`, then enable **Connect to NTRIP server**.

`MOCK1` and `MOCK2` stream data. `MOCK_AUTH` is listed as requiring Basic auth, but authentication
is only enforced with `--auth USER:PASS`, and then for every request. Unknown mount points return
404.

| Option | Default | Description |
| --- | --- | --- |
| `--host` | 127.0.0.1 | Bind address |
| `--port` | 2101 | Port number |
| `--auth` | off | Require Basic auth (`USER:PASS`) |
| `--rate` | 1 | Messages per second |
| `--error` | off | `bad_crc` corrupts every 10th frame, `drop` closes after 10 frames, `http_401`/`http_500` reject every request |

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
| Dependencies | pymavlink | Docker or an ArduPilot build |
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
