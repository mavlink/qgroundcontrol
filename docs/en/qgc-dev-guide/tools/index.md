# Developer Tools

*QGroundControl* makes a number of tools available primarily for autopilot developers.
These ease common developer tasks including setting up simulated connections for testing, 
and accessing the System Shell over MAVLink.

> **Note** [Build the source in debug mode](https://github.com/mavlink/qgroundcontrol#supported-builds) to enable these tools.

Tools include:

- **[Mock Link](../tools/mock_link.md)** (Daily Builds only) - Creates and stops multiple simulated vehicle links.
- **[Replay Flight Data](https://docs.qgroundcontrol.com/en/app_menu/replay_flight_data.html)** - Replay a telemetry log (User Guide).
- **[MAVLink Inspector](https://docs.qgroundcontrol.com/en/app_menu/mavlink_inspector.html)** - Display received MAVLink messages/values.
- **[MAVLink Analyzer](https://docs.qgroundcontrol.com/en/app_menu/mavlink_analyzer.html)** - Plot trends for MAVLink messages/values.
- **[Custom Command Widget](https://docs.qgroundcontrol.com/en/app_menu/custom_command_widget.html)** - Load custom/test QML UI at runtime.
- **[Onboard Files](https://docs.qgroundcontrol.com/en/app_menu/onboard_files.html)** - Navigate vehicle file system and upload/download files.
- **[HIL Config Widget](https://docs.qgroundcontrol.com/en/app_menu/hil_config.html)** - Settings for HIL simulators.
- **[MAVLink Console](https://docs.qgroundcontrol.com/en/analyze_view/mavlink_console.html)** (PX4 Only) - Connect to the PX4 nsh shell and send commands.
