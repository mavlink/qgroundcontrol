# 电机设置

电机设置用于测试单个电机/舵机（例如，验证电机是否按正确方向旋转）。

:::tip
这些指示适用于PX4和ArduPilot上的大多数载具类型。
针对特定飞行器的说明作为子主题提供（例如[电机设置（ArduSub）](../setup_view/motors_ardusub.md)）。
:::

![电机测试](../../../assets/setup/Motors.png)

## 测试步骤

要测试的电机：

1. 卸下所有螺旋桨。

   ::: warning
   在启动电机前，必须取下螺旋桨！
   :::

2. (_PX4-only_) 启用安全开关 - 如果使用该开关的话。

3. 滑动开关以启用电机滑块和按钮（标签为：_已移除螺旋桨 - 启用滑块和电机_）。

4. 调节滑块以选择电机功率。

5. 按下对应的电源按钮，然后确认它朝正确方向旋转。

   ::: info
   3秒后发动机将自动停止旋转。
   您也可以按“停止”按钮来阻止发动机。
   如果没有电机转动，调高 “油门百分比” 后再试。
   :::

## 附加信息

- [基本配置 > Motor Setup](http://docs.px4.io/master/en/config/motors.html) (_PX4 用户指南) - 这包含额外的 PX4 特定信息。
- [电调与电机](https://ardupilot.org/copter/docs/connect-escs-and-motors.html#motor-order-diagrams) - 这是所有机架的电机顺序图
