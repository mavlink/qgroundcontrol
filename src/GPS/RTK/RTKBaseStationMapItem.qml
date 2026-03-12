import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

MapItemGroup {
    id: root

    property var activeVehicle: null
    property bool planView:     false

    property var _gpsRtk: QGroundControl.gpsRtk

    MapQuickItem {
        id:             rtkBaseMarker
        anchorPoint.x:  sourceItem.width / 2
        anchorPoint.y:  sourceItem.height / 2
        visible:        _gpsRtk && _gpsRtk.connected.value
                        && _gpsRtk.baseLatitude.value !== 0
                        && _gpsRtk.baseLongitude.value !== 0
                        && !planView
        coordinate:     visible ? QtPositioning.coordinate(
                            _gpsRtk.baseLatitude.value,
                            _gpsRtk.baseLongitude.value) : QtPositioning.coordinate()

        sourceItem: Rectangle {
            width:  ScreenTools.defaultFontPixelHeight * 2
            height: width
            radius: width / 2
            color:  "transparent"
            border.color: "#FFA500"
            border.width: 2

            Rectangle {
                anchors.centerIn: parent
                width:  parent.width * 0.4
                height: width
                radius: width / 2
                color:  "#FFA500"
            }
        }
    }

    MapPolyline {
        visible:    rtkBaseMarker.visible
                    && activeVehicle
                    && activeVehicle.coordinate.isValid
        line.width: 2
        line.color: "#FFA500"
        opacity:    0.7
        path:       visible ? [rtkBaseMarker.coordinate, activeVehicle.coordinate] : []
    }
}
