# Plan View

- Top level QML code is in [PlanView.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanView.qml)
- Main visual UI is a FlightMap control
- The right panel is a tabbed panel with Plan, Mission, Fence and Rally tabs: [PlanPanel.qml](https://github.com/mavlink/qgroundcontrol/blob/master/src/PlanView/PlanPanel.qml).
  The selected tab sets the map editing layer.
- QML communicates with MissionController (C++) which provides the view with the mission item data and methods
