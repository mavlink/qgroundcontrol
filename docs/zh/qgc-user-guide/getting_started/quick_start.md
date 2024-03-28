# QGroundControl快速上手指南

让 _QGroundControl_ 轻松入门上手使用：

1. [下载并安装](../getting_started/download_and_install.md) 应用程序.
2. 启动_QGroundControl_。
3. Attach your vehicle to the ground station device via USB, through a telemetry radio, or over WiFi. _QGroundControl_ should detect your vehicle and connect to it automatically.

That's it! 就这么简单！ 如果设备准备飞行（无人机是飞，其它设备是运行），_QGroundControl_应显示如下[Fly View](../fly_view/fly_view.md) (否则将打开[Setup View](../setup_view/setup_view.md))。

![](../../../assets/quickstart/fly_view_connected_vehicle.jpg)

要想把_QGroundControl_玩的纯熟，最好方法是亲自上手折腾：

- 使用[工具栏](../toolbar/toolbar.md)在主视图之间切换：
  - [Settings](../settings_view/settings_view.md)：配置 _QGroundControl_ 应用程序。
  - [Setup](../setup_view/setup_view.md)：配置和调试你的设备。
  - [Plan](../plan_view/plan_view.md)：创建自主自动执行的任务
  - [Fly](../fly_view/fly_view.md)：在飞行时监测您的车辆，包括视频流。
  - [Analyze] \*\* Description of Analyze view is missing \*\*
- 点击工具栏上的_Status 图标_来确认已连接设备的状态。

While the UI is fairly intuitive, this documentation can also be referenced to find out more.

:::info
Make sure QGC has an internet connection when you connect a new vehicle. This will allow it to get the latest parameter and other metadata for the vehicle, along with [translations](../settings_view/general.md#miscellaneous).
:::
