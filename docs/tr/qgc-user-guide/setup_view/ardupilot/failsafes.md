# Failsafes (ArduPilot)

The Failsafes page configures automatic safety actions that trigger when critical systems fail. The available failsafe options vary by vehicle type.

## Battery Failsafe

Configurable per battery (if multiple batteries are present):

- **Low battery action** — what to do when the battery reaches low level (RTL / Land / SmartRTL / etc.)
- **Critical battery action** — what to do when the battery reaches critical level
- **Low/Critical voltage thresholds** — voltage levels that trigger each action
- **Low/Critical capacity thresholds** — remaining mAh levels that trigger each action

## Ground Station Failsafe

- **GCS failsafe enable** — action when GCS heartbeat is lost
- **GCS timeout** — time without heartbeat before triggering (Copter/Rover)
- **Trigger method** — Heartbeat only / Heartbeat + RSSI / Heartbeat + AUTO mode (Plane)

## Throttle Failsafe

- **Throttle failsafe enable** — action when RC throttle signal is lost
- **Throttle failsafe value** — PWM value that indicates signal loss
- Additional options vary by vehicle type (short/long action timeouts for Plane, etc.)

## EKF Failsafe

- **EKF failsafe action** — action when the EKF position/velocity estimate becomes unreliable (Land / AltHold / Hold)
- **EKF variance threshold** — EKF variance level that triggers the failsafe

## Other Failsafes

Depending on vehicle type, additional failsafes may include:

- **Dead Reckoning** (Copter) — action and timeout when GPS is lost
- **Crash Check** — detect and respond to crashes
- **Vibration** — detect excessive vibration
- **Leak Detection** (Submarine) — pin and logic level for leak sensors
- **Internal Temperature / Pressure** (Submarine) — environmental limits
