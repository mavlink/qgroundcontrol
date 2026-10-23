# Developer Tools

_QGroundControl_ makes a number of tools available primarily for autopilot developers.
These ease common developer tasks including setting up simulated connections for testing,
and accessing the System Shell over MAVLink.

::: info
[Build the source](../getting_started/index.md) in Debug mode to enable the debug-only tools.
:::

Tools include:

- **[Mock Link](../tools/mock_link.md)** (Debug Builds only) - Creates and stops multiple simulated vehicle links.
- **[Command Line Options](../command_line_options.md)** - Enable logging, run unit tests and simulate other host environments.
- **[Replay Flight Data](../../qgc-user-guide/fly_view/replay_flight_data.md)** - Replay a telemetry log (User Guide).
- **[MAVLink Inspector](../../qgc-user-guide/analyze_view/mavlink_inspector.md)** - Display received MAVLink messages/values and plot trends.
- **[MAVLink Console](../../qgc-user-guide/analyze_view/mavlink_console.md)** (PX4 Only) - Connect to the PX4 nsh shell and send commands.
