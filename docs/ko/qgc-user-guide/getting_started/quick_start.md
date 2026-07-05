# First Flight Guide

This guide walks you through connecting your vehicle, verifying it's ready, and executing your first flight.

## 1. Connect Your Vehicle

Start _QGroundControl_, then connect your vehicle using one of these methods:

- **USB** — plug the flight controller directly into your computer
- **Telemetry radio** — connect a ground-side radio module via USB
- **WiFi / Ethernet** — connect to the vehicle's network (common for companion computers and some commercial drones)

QGC auto-detects most connections. If your vehicle doesn't appear, check **Application Settings > Comm Links** to verify settings or create a manual link.

:::info
Make sure QGC has an internet connection when you connect a new vehicle so the map loads correctly.
:::

## 2. Check Vehicle Status

Once connected, look at the **toolbar indicators** across the top of the screen. Each icon shows a subsystem status — tap any indicator for details:

- **GPS** — satellite count and fix type (3D fix required for most flight modes)
- **Battery** — voltage and remaining capacity
- **RC** — transmitter connection status
- **Telemetry** — link signal quality
- **Flight Mode** — current mode (tap to change)
- **Armed/Disarmed** — arming state (tap to arm/disarm)

## 3. Calibrate (New Vehicles)

For a new vehicle, QGC opens [Vehicle Configuration](../setup_view/setup_view.md) automatically. Complete these steps in order:

1. **[Firmware](../setup_view/firmware.md)** — install or update flight controller firmware (USB only)
2. **Airframe / Frame** — select your vehicle type and frame layout
3. **Sensors** — calibrate compass, accelerometer, gyroscope, and (if equipped) level horizon
4. **Radio** — calibrate your RC transmitter stick ranges
5. **Flight Modes** — assign flight modes to transmitter switches
6. **Power** — configure battery cell count and voltage sensing

Parameters and other settings can be adjusted later. See the [PX4](../setup_view/sensors_px4.md) or [ArduPilot](../setup_view/sensors_ardupilot.md) sensor pages for firmware-specific guidance.

## 4. Plan a Simple Mission (Optional)

Open the **Planning Missions** view to create a basic waypoint mission:

1. Tap the map to place a takeoff point
2. Tap additional locations to add waypoints
3. Set the desired altitude for each waypoint (or accept the default)
4. Upload the mission to the vehicle

See [Planning Missions](../plan_view/plan_view.md) for details on surveys, geofences, and rally points.

## 5. Fly

Switch to the **Flying** view. You should see a map centered on your vehicle's GPS position with the HUD overlay showing attitude.

**To fly a mission:**

1. Confirm all toolbar indicators are green/nominal
2. Slide to arm (or tap the arm indicator)
3. Tap **Start Mission** — the vehicle will take off and follow the planned route
4. Monitor progress in the toolbar and map
5. When the mission completes, the vehicle returns to launch and lands automatically (depending on firmware settings)

**To fly manually:**

1. Arm the vehicle via QGC or your transmitter
2. Take off using your transmitter
3. Use the **Fly Tools** (left edge of the map) for guided actions: Return to Launch, Land, Pause, Change Altitude, Go To Location, Orbit

## 6) After Landing

- **Review telemetry** — open [Analysis & Logs](../analyze_view/index.md) to download and inspect flight logs
- **Download logs** — use [Log Download](../analyze_view/log_download.md) to pull logs from the vehicle for post-flight analysis

## Troubleshooting

- Vehicle not connecting? See [Connection Problems](../troubleshooting/vehicle_connection.md)
- Parameters won't download? See [Parameter Download Failures](../troubleshooting/parameter_download.md)
- Mission won't upload? See [Plan Upload/Download Failures](../troubleshooting/plan_upload_download.md)
