# MAVLink 控制台(分析视图)

MAVLink 控制台 (**Analyze > Mavlink 控制台**) 允许您连接到 PX4 [系统控制台](https://docs.px4.io/main/en/debug/system_console.html) 并发送命令。

:::info
它仅在桌面版本上支持 (Windows, Linux, Mac OS)。
不支持PX4 SITL和ArduPilot 。
:::

:::tip
这是开发人员非常有用的功能，因为它允许深入访问系统。 特别是，如果你通过Wifi连接，你可以在载具飞行时拥有这个相同的访问级别。
:::

![分析视图MAVLink控制台](../../../assets/analyze/mavlink_console.jpg)

除了对命令的响应外，视图不显示任何输出。
一旦车辆连接，您可以在所提供的条形中输入命令(输入可用命令的完整列表：`?`)。

命令输出显示在命令栏上方的视图中。
点击“显示最新内容”跳转到命令输出的底部。
