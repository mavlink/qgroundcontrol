# 开发者工具

_QGroundControl_ 提供了一系列主要面向自动驾驶仪开发者的工具。这些简化了常见的开发人员任务，包括设置模拟连接以进行测试，以及通过MAVLink访问系统Shell。

:::info
[Build the source](../getting_started/index.md) in Debug mode to enable the debug-only tools.
:::

工具包括：

- **[Mock Link](../tools/mock_link.md)** (仅调试版本) - 创建和停止多个模拟载具链接。
- **[Command Line Options](../command_line_options.md)** - Enable logging, run unit tests and simulate other host environments.
- **[重播飞行数据](../../qgc-user-guide/fly_view/replay_flight_data.md)** - 重播一个遥测日志(用户指南)。
- **[MAVLink Inspector](../../qgc-user-guide/analyze_view/mavlink_inspector.md)** - Display received MAVLink messages/values and plot trends.
- **[MAVLink Console](../../qgc-user-guide/analyze_view/mavlink_console.md)** (PX4 Only) - Connect to the PX4 nsh shell and send commands.
