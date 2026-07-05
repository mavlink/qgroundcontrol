# Servo Outputs (ArduPilot)

The Servo Outputs page provides real-time visualization and configuration for up to 16 servo/motor output channels.

## Per-Channel Controls

Each servo channel (if its `SERVOn_FUNCTION` parameter exists) displays:

| Column       | Description                                                                                                                           | Parameter                                |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------------- |
| **#**        | Servo number (1–16)                                                                                                | —                                        |
| **Position** | Live PWM value shown as a progress bar with numeric readout                                                                           | (read-only telemetry) |
| **Function** | Output function assignment (e.g., Motor1, Aileron, Throttle, etc.) | `SERVOn_FUNCTION`                        |
| **Min**      | Minimum PWM output (adjustable with +/− buttons)                                                                   | `SERVOn_MIN`                             |
| **Trim**     | Neutral/center PWM value (adjustable with +/− buttons)                                                             | `SERVOn_TRIM`                            |
| **Max**      | Maximum PWM output (adjustable with +/− buttons)                                                                   | `SERVOn_MAX`                             |
| **Reversed** | Reverse the output direction                                                                                                          | `SERVOn_REVERSED`                        |

The live position bars update in real time from the vehicle's servo output telemetry, allowing you to verify that outputs respond correctly to control inputs.
