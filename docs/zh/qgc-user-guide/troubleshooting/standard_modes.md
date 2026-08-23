# Standard Modes / External Flight Mode failures

Standard Modes is a [MAVLink microservice](https://mavlink.io/en/services/standard_modes.html) through which supporting firmware (including PX4 and ArduPilot) advertises its flight modes to _QGroundControl_ during the initial connection, using the `AVAILABLE_MODES`, `AVAILABLE_MODES_MONITOR` and `CURRENT_MODE` messages.
This includes externally-registered flight modes.

If an expected flight mode does not appear in the flight-mode selector (or the wrong mode name is shown), the information needed to debug the problem is _QGroundControl's_ internal processing of these messages.
A MAVLink traffic capture alone is not sufficient, since it does not show how _QGroundControl_ processed the messages.

## Capturing a Standard Modes log

1. Open **Application Settings > App Log Viewer** and click **Categories**.
2. Enable the `Vehicle.StandardModes` logging category.
3. Reproduce the problem: connect the vehicle (with any external modes already registered) and wait for the connection sequence to complete.
4. Back in the App Log Viewer, click **Save** to save the application log to a file.

The log shows each `AVAILABLE_MODES` mode as it is received (name, index, standard mode, custom mode, and flags), when mode retrieval completes, and any request failures or re-requests triggered by `AVAILABLE_MODES_MONITOR` changes.

## Reporting an issue

When [reporting an issue](../support/support.md), attach the saved _QGroundControl_ app log captured as described above.
A MAVLink capture may be attached in addition, but is not a substitute for the app log.
