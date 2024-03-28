# 开发者工具

QGroundControl主要为自动驾驶开发人员提供了许多工具。 这些简化了常见的开发人员任务，包括设置模拟连接以进行测试，以及通过MAVLink访问系统Shell。
These ease common developer tasks including setting up simulated connections for testing,
and accessing the System Shell over MAVLink.

:::info
[Build the source in debug mode](https://github.com/mavlink/qgroundcontrol#supported-builds) to enable these tools.
:::

工具包括：

- 模拟链接（仅限每日构建） - 创建和停止多个模拟载具链接。
- **[Replay Flight Data](../../qgc-user-guide/fly_view/replay_flight_data.md)** - Replay a telemetry log (User Guide).
- **[MAVLink Inspector](../../qgc-user-guide/analyze_view/mavlink_inspector.html)** - Display received MAVLink messages/values and plot trends.
- **[MAVLink Console](../../qgc-user-guide/analyze_view/mavlink_console.html)** (PX4 Only) - Connect to the PX4 nsh shell and send commands.
