# 规划视图

- Top level QML code is in [PlanView.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanView.qml)
- 主视图UI是FlightMap控件
- The right panel is a unified plan tree: [PlanTreeView.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanTreeView.qml)
- QML与MissionController（C ++）通信，后者为视图提供任务项数据和方法
