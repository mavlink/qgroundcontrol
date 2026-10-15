import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Survey Complex Mission Item visuals
TransectStyleMapVisuals {
    polygonInteractive: true
    transectColor: QGroundControl.globalPalette.colorYellow
    transectLineWidth: ScreenTools.defaultFontPixelWidth / 3
    showWaypointNumbers: true
    waypointMarkers: _missionItem.waypointMarkers
}
