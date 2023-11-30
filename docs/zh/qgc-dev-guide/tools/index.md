# 开发者工具 

QGroundControl主要为自动驾驶开发人员提供了许多工具。 这些简化了常见的开发人员任务，包括设置模拟连接以进行测试，以及通过MAVLink访问系统Shell。

> 注意:在调试模式下构建源以启用这些工具。

工具包括：

- 模拟链接（仅限每日构建） - 创建和停止多个模拟载具链接。
- 重播飞行数据 - 重播遥测日志（用户指南）。
- MAVLink Inspector - 显示收到的MAVLink消息/值。
- MAVLink分析器 - 绘制MAVLink消息/值的趋势图。
- 自定义命令小组件 - 在运行时加载自定义/测试QML UI。
- 板载文件 - 导航车辆文件系统和上载/下载文件。
- HIL Config Widget - HIL模拟器的设置.
- MAVLink控制台（仅限PX4） - 连接到PX4 nsh shell并发送命令。