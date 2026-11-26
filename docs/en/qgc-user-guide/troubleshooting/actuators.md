# Missing Actuators Page (PX4)

The [Actuators page](../setup_view/px4/actuators.md) is shown when QGroundControl receives and initializes the
vehicle's `COMP_METADATA_TYPE_ACTUATORS` component metadata. The firmware
version alone does not determine whether the page is available. If the
metadata is unavailable, cannot be initialized, or its top-level `show-ui-if`
condition evaluates to false, QGroundControl uses the legacy Motors page
instead.

## Capture Diagnostic Logs

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

The metadata URI can use MAVLink FTP (`mftp://`) or HTTP(S). QGroundControl
refuses any other URI, such as a local path or a `file://` URL. It
tries the primary URI first and then the fallback URI advertised by the
vehicle.

## Read the Diagnostic Messages

| Log category | Message or pattern | Meaning |
| --- | --- | --- |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Vehicle did not provide actuators metadata via component information` | No actuator metadata was received from the vehicle. |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Actuators initialization failed` | Metadata was received but could not be parsed or initialized. |
| `Vehicle.Actuators.PX4AutoPilotPlugin` | `Condition 'show-ui-if' evaluated to false` | The vehicle's metadata indicated the page should not be shown. This can be intentional for your configuration, or a firmware/metadata mismatch. Attach the saved log to a support request. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `primary failed, requesting metadata (fallback) from ...` | The primary metadata URI failed and QGroundControl is trying the fallback URI. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `download failed: ...` | The metadata could not be downloaded over MAVLink FTP or HTTP. An HTTP status such as `404` identifies a missing or incorrect URL; `larger than` means the file exceeds the 64 MiB that QGroundControl accepts. |
| `Vehicle.VehicleFileFetch` | `Cannot fetch ... - only MAVLink FTP and HTTP(S) are fetched` | The URI is not a MAVLink FTP or HTTP(S) URI, so QGroundControl does not download it. This warning is logged without enabling the category. |
| `ComponentInformation.RequestMetaDataTypeStateMachine` | `failed to load metadata (primary and fallback)` | Both metadata URIs failed, so the Actuators page cannot be initialized. |
| `Vehicle.FTPManager` | `Download fromCompId`, `Nak - ...`, or timeout messages | Details of the MAVLink FTP request and any vehicle-side error. |

## Include This Information in a Support Request

Include the QGroundControl version and build, PX4 firmware version, vehicle
type, connection type, whether the legacy Motors page appears, and the saved
app log. If the log contains a fallback or download error, include the full
line with the URI and error text. Also mention whether reconnecting the vehicle
changes the result.
