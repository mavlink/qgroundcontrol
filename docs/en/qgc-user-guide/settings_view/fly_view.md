# Fly View Settings

Settings that control the behavior and appearance of the [Fly View](../fly_view/fly_view.md).

## General

- **Use preflight checklist** — enable the pre-flight checklist tool in the Fly View
- **Enforce checklist** — prevent arming until all checklist items are complete
- **Enable multi-vehicle panel** — show the multi-vehicle list when more than one vehicle is connected
- **Keep Map Centered On Vehicle** — prevent the map from drifting away from the vehicle position
- **Show Telemetry Log Replay Status Bar** — display the replay control bar when playing back telemetry logs
- **Show simple camera controls (DIGICAM_CONTROL)** — show basic camera trigger controls for autopilot-connected cameras
- **Update return to home position based on device location** — send the GCS position as the vehicle's home/RTL point
- **Enable automatic mission start/resume popups** — show mission start and resume prompts automatically

## Guided Commands

- **Minimum Altitude** — lowest altitude allowed for guided mode commands (default: 2 m)
- **Maximum Altitude** — highest altitude allowed for guided mode commands (default: 121.92 m / 400 ft)
- **Go To Location Max Distance** — maximum distance for Go To commands (default: 1000 m)
- **Loiter Radius in Forward Flight Guided Mode** — loiter radius for fixed-wing guided mode (default: 80 m)
- **Require Confirmation for Go To Location in Guided Mode** — require slider confirmation before executing Go To

## MAVLink Actions

- **Fly View Actions file** — JSON file defining custom MAVLink actions available in the Fly View
- **Joystick Actions file** — JSON file defining custom MAVLink actions triggered by joystick buttons

## Virtual Joystick

- **Virtual joystick** — enable on-screen virtual thumbsticks for vehicle control
- **Auto-center throttle** — return throttle to center when released
- **Left-handed mode** — swap the throttle and direction sticks

## Instrument Panel

- **Show additional heading indicators on Compass** — display extra heading markers
- **Lock Compass Nose-Up** — keep the compass oriented nose-up instead of rotating with heading
- **Instrument panel style** — choose between Integrated, Horizontal, or Large Vertical layouts
