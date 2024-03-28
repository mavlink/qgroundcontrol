# Developer Tools

_QGroundControl_ makes a number of tools available primarily for autopilot developers.
These ease common developer tasks including setting up simulated connections for testing,
and accessing the System Shell over MAVLink.

::: info
[Build the source in debug mode](https://github.com/mavlink/qgroundcontrol#supported-builds) to enable these tools.
:::

Tools include:

- **[Mock Link](../tools/mock_link.md)** (Daily Builds only) - Creates and stops multiple simulated vehicle links.
- **[Replay Flight Data](../../qgc-user-guide/fly_view/replay_flight_data.md)** - Replay a telemetry log (User Guide).
- **[MAVLink Inspector](../../qgc-user-guide/analyze_view/mavlink_inspector.html)** - Display received MAVLink messages/values and plot trends.
- **[MAVLink Console](../../qgc-user-guide/analyze_view/mavlink_console.html)** (PX4 Only) - Connect to the PX4 nsh shell and send commands.
