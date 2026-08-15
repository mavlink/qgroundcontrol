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

::: warning
Remove all propellers before testing actuators.
:::

With the safety switch toggled, individual actuator sliders appear for testing each channel. An optional **All Motors** slider allows spinning all motors simultaneously at low throttle for verification.

## Actuator Actions

Dynamic action buttons (e.g., beep, identify, 3D mode) are shown based on the vehicle's actuator configuration. The **motor identification** feature spins motors one at a time to verify correct wiring and rotation direction.

## Troubleshooting a Missing Actuators Page

The Actuators page is shown when QGroundControl receives and initializes the
vehicle's `COMP_METADATA_TYPE_ACTUATORS` component metadata. The firmware
version alone does not determine whether the page is available. If the
metadata is unavailable, cannot be initialized, or its top-level `show-ui-if`
condition evaluates to false, QGroundControl uses the legacy Motors page
instead.

### Capture Diagnostic Logs

1. Open **Application Settings > App Log Viewer**.
2. Click **Categories** and enable these categories:
   - `Vehicle.Actuators.PX4AutoPilotPlugin`
   - `Vehicle.Actuators.Common`
   - `ComponentInformation.RequestMetaDataTypeStateMachine`
   - `Vehicle.FTPManager`
3. Connect the vehicle. If it is already connected, disconnect and reconnect
   it so that QGroundControl requests the component metadata again.
4. Open **Vehicle Setup** and check whether **Actuators** or **Motors** is
   available.
5. Return to **App Log Viewer** and click **Save**. Keep the saved app log
   unchanged when attaching it to a support request.

The metadata URI can use MAVLink FTP (`mavlinkftp://`) or HTTP. QGroundControl
tries the primary URI first and then the fallback URI advertised by the
vehicle.

### Read the Diagnostic Messages

| Log category | Message or pattern | Meaning |
| --- | --- | --- |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Vehicle did not provide actuators metadata via component information` | No actuator metadata was received from the vehicle. |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Actuators initialization failed` | Metadata was received but could not be parsed or initialized. |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Condition 'show-ui-if' evaluated to false` | The vehicle's metadata indicated the page should not be shown. This can be intentional for your configuration, or a firmware/metadata mismatch. Attach the saved log to a support request. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `primary failed, requesting metadata (fallback) from ...` | The primary metadata URI failed and QGroundControl is trying the fallback URI. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `FTP download failed` or `HTTP download failed` | The selected transport could not download the metadata. An HTTP status such as `404` identifies a missing or incorrect URL. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `failed to load metadata (primary and fallback)` | Both metadata URIs failed, so the Actuators page cannot be initialized. |
| `Vehicle.FTPManager` | `Download fromCompId`, `Nak - ...`, or timeout messages | Details of the MAVLink FTP request and any vehicle-side error. |

### Include This Information in a Support Request

Include the QGroundControl version and build, PX4 firmware version, vehicle
type, connection type, whether the legacy Motors page appears, and the saved
app log. If the log contains a fallback or download error, include the full
line with the URI and error text. Also mention whether reconnecting the vehicle
changes the result.
