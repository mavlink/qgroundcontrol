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

1. Create a debug build by [building the source](https://github.com/mavlink/qgroundcontrol#supported-builds).

2. 通过选择顶部工具栏中的“应用程序设置”图标，然后选择侧栏中的“模拟链接”来访问“模拟链接”：

   ![](../../../assets/dev_tools/mocklink_waiting_for_connection.jpg)

3. 可以单击面板中的按钮以创建相关类型的车辆链接。

   - 每次单击按钮时，都会创建一个新连接。
   - 当存在多个连接时，将显示多车辆UI。
     ![](../../../assets/dev_tools/mocklink_connected.jpg)

   ![](../../../assets/dev_tools/mocklink_connected.jpg)

4. 单击停止一个模拟链接以停止当前活动的车辆。

然后使用模拟链接或多或少与使用任何其他载具相同，只是模拟不允许飞行。
