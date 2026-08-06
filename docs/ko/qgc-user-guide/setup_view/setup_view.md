# Vehicle Configuration

:::tip Having trouble?
If QGC won't connect to your vehicle, see [Connection Problems](../troubleshooting/vehicle_connection.md). If parameters fail to load, see [Parameter Download Failures](../troubleshooting/parameter_download.md).
:::

The Vehicle Configuration view is used to configure a new vehicle prior to first flight and/or tune a configured vehicle.

## Search

Use the **search bar** at the top of the configuration sidebar to find any setting by name. As you type, matching settings are shown across all pages — click a result to jump directly to that setting.

## Configuration Pages

To the left of the screen are the available configuration pages. A page is marked with a red icon if there are still settings needed to be adjusted/specified. 이 중 하나라도 빨간색이면 비행해서는 안 됩니다.

:::info
The set of pages shown and the contents of each page may differ based on whether the vehicle is running PX4 or ArduPilot firmware.
:::

**Summary** <br>An overview of all the important configuration options for your vehicle. Summary blocks show a red indicator when settings are not fully configured.

**PX4-specific pages:** [Actuators / Motors](px4/actuators.md), [Airframe](airframe_px4.md), [Flight Modes](flight_modes_px4.md), [PID Tuning](tuning_px4.md), [Power](power.md), [Radio](radio.md), [Safety](safety.md), [Sensors](sensors_px4.md)

**ArduPilot-specific pages:** [ESC](ardupilot/esc.md), [Failsafes](ardupilot/failsafes.md), [Flight Modes](flight_modes_ardupilot.md), [Flight Safety](ardupilot/flight_safety.md), [Frame](airframe_ardupilot.md), [Gimbal](camera.md), [Logging](logging_ardupilot.md), [Motors](motors.md), [Power](power.md), [Radio](radio.md), [Remote Support](ardupilot/remote_support.md), [Scripting](ardupilot/scripting.md), [Sensors](sensors_ardupilot.md), [Servo Outputs](ardupilot/servo_outputs.md), [Tuning](tuning_ardupilot.md), [Vehicle-Specific](ardupilot/vehicle_specific.md)

**Shared pages:**

**[Joystick](joystick.md)** <br>Configure a joystick or gamepad for vehicle control.

**[Firmware](firmware.md)** <br>Flash new firmware onto the flight controller.

**[Parameters](parameters.md)** <br>View and modify all parameters associated with your vehicle.
