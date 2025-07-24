# 开发者工具

_QGroundControl_ 提供了一系列主要面向自动驾驶仪开发者的工具。
这些简化了常见的开发人员任务，包括设置模拟连接以进行测试，以及通过MAVLink访问系统Shell。

:::info
[在调试模式中生成源](https://github.com/mavlink/qgroundcontrol#supported-builds) 以启用这些工具。
:::

工具包括：

- **[Mock Link](../tools/mock_link.md)** (仅调试版本) - 创建和停止多个模拟载具链接。
- **[重播飞行数据](../../qgc-user-guide/fly_view/replay_flight_data.md)** - 重播一个遥测日志(用户指南)。
- **[MAVLink 检查器](../../qgc-user-guide/analyze_view/mavlink_inspector.html)** - 显示已收到的 MAVLink 消息/值和绘图趋势。
- **[MAVLink 控制台](../../qgc-user-guide/analyze_view/mavlink_console.html)** （仅限PX4） - 连接到 PX4 nsh shell 并发送命令。
