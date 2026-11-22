# Fly View

- Top level QML code is in **FlightDisplayView.qml**
- QML code communicates with `MissionController` (C++) for mission display
- Instrument widgets communicate with active Vehicle object
- Two main inner views are:
  - `FlightDisplayViewMap`
  - `FlyViewVideo`, around the `VideoView` of the `QGroundControl.VideoManager` QML module (`src/VideoManager/Qml`),
    which shows the other video sources in tiles (`VideoTiles`) while the video is the main view
