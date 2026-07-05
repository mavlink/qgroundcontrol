# Actuators / Motors (PX4)

The Actuators page provides a unified interface for configuring motor and servo outputs on PX4 vehicles (PX4 v1.13+).

## Geometry / Mixer

The top section displays the vehicle's actuator geometry, dynamically generated from the mixer configuration. For multirotor vehicles, an SVG diagram shows motor positions — you can click individual motors to select them. Each actuator channel has configurable parameters such as position offsets and tilt angles.

## Actuator Outputs

The right panel shows all output channels with:

- **Function** — assign each output to a motor, servo, or other function
- **Min / Max** — PWM or DShot output range limits
- **Failsafe** — output value when failsafe is triggered
- **Disarmed** — output value when the vehicle is disarmed

## Actuator Testing

:::warning
Remove all propellers before testing actuators.
:::

With the safety switch toggled, individual actuator sliders appear for testing each channel. An optional **All Motors** slider allows spinning all motors simultaneously at low throttle for verification.

## Actuator Actions

Dynamic action buttons (e.g., beep, identify, 3D mode) are shown based on the vehicle's actuator configuration. The **motor identification** feature spins motors one at a time to verify correct wiring and rotation direction.
