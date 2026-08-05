# ESC Calibration (ArduPilot)

The ESC page configures Electronic Speed Controller output parameters and provides ESC calibration for ArduPilot vehicles.

## Configuration

- **Output type** — PWM output protocol: Normal / OneShot / OneShot125 / DShot150/300/600/1200 (`MOT_PWM_TYPE`). For QuadPlane VTOL, uses `Q_M_PWM_TYPE`.
- **Output PWM min** — minimum PWM value sent to ESCs (`MOT_PWM_MIN`)
- **Output PWM max** — maximum PWM value sent to ESCs (`MOT_PWM_MAX`)
- **Spin when armed** — motor spin level when armed but not flying (`MOT_SPIN_ARM`)
- **Spin minimum** — minimum spin level during flight (`MOT_SPIN_MIN`)
- **Spin maximum** — maximum spin level (`MOT_SPIN_MAX`)

When a DShot output type is selected, additional settings appear:

- **DShot ESC type** — ESC protocol variant (`SERVO_DSHOT_ESC`)
- **DShot output rate** — DShot frame rate (`SERVO_DSHOT_RATE`)

## Calibration

:::warning
Remove all propellers before calibrating ESCs.
:::

Press the **Calibrate** button to run the ESC calibration procedure. This sets `ESC_CALIBRATION = 3`, which triggers the ESC to learn the throttle range on the next reboot.
