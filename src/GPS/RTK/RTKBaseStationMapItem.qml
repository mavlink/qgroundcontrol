import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

MapItemGroup {
    id: root

    property var activeVehicle: null
    property bool planView:     false

    property var _gpsMgr: QGroundControl.gpsManager

    QGCPalette { id: qgcPal }

    Repeater {
        model: _gpsMgr ? _gpsMgr.devices : null

        MapItemGroup {
            required property var object

            property var _fg: object ? object.factGroup : null
            property bool _visible: Boolean(_fg)
                                    && Boolean(_fg.connected && _fg.connected.value)
                                    && Boolean(_fg.baseLatitude && _fg.baseLatitude.value !== 0)
                                    && Boolean(_fg.baseLongitude && _fg.baseLongitude.value !== 0)
                                    && !planView

            MapQuickItem {
                anchorPoint.x:  sourceItem.width / 2
                anchorPoint.y:  sourceItem.height / 2
                visible:        _visible
                coordinate:     _visible ? QtPositioning.coordinate(
                                    _fg.baseLatitude.value,
                                    _fg.baseLongitude.value) : QtPositioning.coordinate()

                sourceItem: Rectangle {
                    width:  ScreenTools.defaultFontPixelHeight * 2
                    height: width
                    radius: width / 2
                    color:  "transparent"
                    border.color: qgcPal.colorOrange
                    border.width: 2

                    Rectangle {
                        anchors.centerIn: parent
                        width:  parent.width * 0.4
                        height: width
                        radius: width / 2
                        color:  qgcPal.colorOrange
                    }
                }
            }

            MapPolyline {
                visible:    _visible && activeVehicle && activeVehicle.coordinate.isValid
                line.width: 2
                line.color: qgcPal.colorOrange
                opacity:    0.7
                path:       visible ? [QtPositioning.coordinate(_fg.baseLatitude.value, _fg.baseLongitude.value),
                                       activeVehicle.coordinate] : []
            }
        }
    }
}
