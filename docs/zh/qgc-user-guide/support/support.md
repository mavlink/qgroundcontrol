# 支持

本用户指南旨在成为 _QGroundControl_ 的主要支持文档。
如果您发现信息不正确或缺失，请提交 [Issue](https://github.com/mavlink/qgc-user-guide/issues)。

关于如何使用 _QGroundControl_ 的问题，应该在相关飞行堆栈的讨论论坛上提出：

- [PX4 Pro Flight Stack](http://discuss.px4.io/c/qgroundcontrol/qgroundcontrol-usage) (discuss.px4.io)。
- [ArduPilot Flight Stack](http://discuss.ardupilot.org/c/ground-control-software/qgroundcontrol) (discuss.ardupilot.org)。

这些论坛也主要靠其他QGC社区成员进行互助。 QGC 的开发人员对它们进行的监管非常有限。

### 开发者交流 {#developer_chat}

_QGroundControl_ 开发者（以及许多正规/深度参与的用户）可以在 [Discord上的 #QGroundControl 频道](https://discord.gg/dronecode) 上找到。

## GitHub Issues

Issues 用于追踪在 _QGroundControl_ 中的 bug 以及以后版本的新特性请求。 当前问题列表可以在 [GitHub](https://github.com/mavlink/qgroundcontrol/issues) 上找到。

:::info
无论是报告错误还是提出功能请求，请在创建 GitHub Issues **之前**，通过支持论坛联系我们的开发人员。
:::

### 报告问题

如果您被引导来创建一个问题，请使用“错误报告”模板并提供模板中指定的所有信息。

##### 报告来自 Windows Builds 的故障

当QGC 崩溃时，将在用户本地应用数据目录中放置崩溃转储文件。 要导航到该目录，请使用启动/运行命令。 你可以通过按下 Win 键 + R 调出此窗口。 在其中输入%localappdata%，点击 “打开”，然后点击 “确定” 。 崩溃转储将出现在该目录的 `QGCCrashDumps` 文件夹中。 您应该在那里找到一个新的 **.dmp** 文件。 在报告问题时在 GitHub Issue 中添加一个链接到该文件。

##### 报告 Windows 版本中的程序挂起问题

如果Windows告诉您_QGroundControl_ 程序不响应，则使用以下步骤报告挂起问题：

1. 打开 _任务管理器_ (右击任务栏，选择 **任务管理器**)
2. 切换到进程标签和本地 **qgroundcontrol.exe**
3. 右键点击**groundcontrol.exe** 并选择 **创建转储文件**
4. 将转储文件放置在一个公共位置
5. 在 GitHub 问题上添加一个 **.dmp** 文件及更多详细信息的链接。

### 功能请求

如果您被指示在讨论支持论坛后创建功能请求，请使用“功能请求”模板，该模板有一些有用的信息说明所需的详细信息。

## 故障处理

故障排查信息可从[此链接](../troubleshooting/index.md)获取。

### 控制台日志

_控制台日志_ 可以帮助诊断_QGroundControl_ 问题。 更多信息请查看：[控制台日志记录](../settings_view/console_logging.md)。

## 帮助改善这些文档！

就像 _QGroundControl_ 本身一样，本用户指南也是开源的，由用户创建并维护，基于 GitBook 平台。 我们欢迎[Pull Requests](https://github.com/mavlink/qgc-user-guide/pulls)以进行修正 和/或 更新。
