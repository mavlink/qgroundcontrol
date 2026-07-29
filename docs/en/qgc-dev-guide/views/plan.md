# Plan View

- Top level QML code is in [PlanView.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanView.qml)
- Main visual UI is a FlightMap control
- The right panel is a unified plan tree: [PlanTreeView.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanTreeView.qml)
- QML communicates with MissionController (C++) which provides the view with the mission item data and methods
