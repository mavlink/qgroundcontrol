# ESC (PX4)

The ESC page calibrates ESCs that take a PWM signal and configures the DShot and DroneCAN ESC settings.
It is designed for PX4 1.17 or later. Older firmware shows only the settings it has.

The page only shows settings the connected vehicle has, and only the groups that apply to its motor outputs.
The output protocol (PWM, OneShot or DShot) and the per-channel output limits are set on the [Actuators](actuators.md) page.
The choices, units and limits of each setting come from the vehicle's parameter metadata.

Settings that only take effect after the vehicle reboots are marked _Applies after reboot_.
Once one is changed from the value it had when the page opened, the mark becomes a _Reboot required_ warning.
The page shows these marks instead of the reboot notice that QGroundControl shows elsewhere. Reboot from the main status indicator in the toolbar.

## Calibration

Calibration teaches ESCs that take a PWM signal the full throttle range of the flight controller.
DShot and DroneCAN ESCs do not need it, so the section is hidden when every output group is set to DShot.

To calibrate:

1. Remove the propellers.
1. Connect the vehicle to QGroundControl with USB only, with the battery disconnected.
1. Click **Calibrate**.
1. Connect the battery when the prompt asks for it, and wait for calibration to finish.

::: warning
Never attempt ESC calibration with propellers fitted.

Motors should not spin during ESC calibration.
However, if an ESC doesn't properly support the calibration sequence it will respond to the PWM input by running the motor at maximum speed.
:::

## DShot

These settings are shown while an output group is set to DShot.

- **Output** — the minimum output, which should be high enough that the motors keep spinning while armed.
  _3D mode_ lets the motors spin in both directions. It needs ESCs set to 3D mode and the minimum output set to 0.
  The two deadband settings are shown while 3D mode is on.
- **Telemetry** — the data the ESCs return over the DShot signal wire.
  PX4 1.17 has _Bidirectional DShot_, which returns RPM and needs ESCs that support it, such as BLHeli32.
  Newer PX4 has _Extended DShot telemetry_, which also returns ESC temperature, voltage and current.
  _ESC firmware_ lets the flight controller read ESC settings over the telemetry wire.
  The motor pole count turns the ESC's electrical RPM into motor RPM: PX4 1.17 has one setting for all motors, newer PX4 has one per motor.
  Telemetry over a separate serial wire is set on the [Ports](ports.md) page.

## DroneCAN

**ESC messages** chooses which CAN interfaces carry ESC messages.
They are sent often at high priority, so use only the interfaces which have ESCs on them.
