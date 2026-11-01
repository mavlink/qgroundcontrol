# 模拟链接

Mock Link允许您在QGroundControl调试版本中创建和停止指向多个模拟（模拟）载具的链接。

模拟不支持飞行，但允许轻松测试：

- 任务上传/下载
- 查看和更改参数
- 测试大多数设置页面
- 多个载具用户界面

对于任务上载/下载的单元测试错误情况尤其有用。

## 使用Mock Link

为了使用Mock Link：

1. Create a Debug build by [building the source](../getting_started/index.md).

2. Access _Mock Link_ by selecting the _Application Settings_ icon in the top toolbar and then **Mock Link** in the sidebar.

3. Choose the **Vehicle Type** (PX4, ArduPilot Copter/Plane/Sub/Rover, or Generic) and **Home Location**, and optionally a served video stream and simulated camera, gimbal, proximity sensors or ADS-B vehicles.

4. Click **Start MockLink** to create a vehicle link with those settings.

   - Each time you click the button a new connection will be created.
   - 当存在多个连接时，将显示多载具UI。

5. Click **Stop One MockLink** to stop the currently active vehicle.

然后使用模拟链接或多或少与使用任何其他载具相同，只是模拟不允许飞行。

:::tip
To test with a vehicle that flies, use a SITL simulator. [`tools/simulation`](https://github.com/mavlink/qgroundcontrol/tree/master/tools/simulation) in the source tree has a lightweight MAVLink vehicle simulator and an ArduCopter SITL launcher.
:::
