import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: root

    property var map
    property bool largeMapView

    property var _targetMarker: null
    property var _targetLine: null

    Component {
        id: targetMarkerComponent

        MapQuickItem {
            id: targetMarker

            visible: atlasPayload.trackingModeEnabled && atlasPayload.targetValid
            z: QGroundControl.zOrderTopMost
            coordinate: QtPositioning.coordinate(
                atlasPayload.targetLatitude,
                atlasPayload.targetLongitude
            )

            anchorPoint.x: markerBody.width / 2
            anchorPoint.y: markerBody.height / 2

            sourceItem: Rectangle {
                id: markerBody
                width: root.largeMapView ? 54 : 38
                height: width
                radius: width / 2
                color: "#CCB00020"
                border.width: 3
                border.color: "#FF3040"

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width * 0.38
                    height: width
                    radius: width / 2
                    color: "#FF3040"
                }

                QGCLabel {
                    anchors.top: parent.bottom
                    anchors.topMargin: 3
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: atlasPayload.targetId >= 0
                          ? "#" + atlasPayload.targetId + "  " + atlasPayload.targetDistanceM.toFixed(1) + " m"
                          : "TARGET"
                    color: "#FF4050"
                    font.bold: true
                    font.pointSize: ScreenTools.smallFontPointSize * 0.9
                }
            }
        }
    }

    Component {
        id: targetLineComponent

        MapPolyline {
            visible: atlasPayload.trackingModeEnabled &&
                     atlasPayload.targetValid &&
                     QGroundControl.multiVehicleManager.activeVehicle
            z: QGroundControl.zOrderTrajectoryLines + 1
            line.width: root.largeMapView ? 3 : 2
            line.color: "#FF3040"

            path: {
                var vehicle = QGroundControl.multiVehicleManager.activeVehicle
                if (!vehicle || !atlasPayload.trackingModeEnabled || !atlasPayload.targetValid) {
                    return []
                }

                return [
                    vehicle.coordinate,
                    QtPositioning.coordinate(
                        atlasPayload.targetLatitude,
                        atlasPayload.targetLongitude
                    )
                ]
            }
        }
    }

    Component.onCompleted: {
        if (map) {
            _targetLine = targetLineComponent.createObject(map)
            _targetMarker = targetMarkerComponent.createObject(map)

            if (_targetLine) {
                map.addMapItem(_targetLine)
            }
            if (_targetMarker) {
                map.addMapItem(_targetMarker)
            }
        }
    }

    Component.onDestruction: {
        if (_targetMarker) {
            _targetMarker.destroy()
        }
        if (_targetLine) {
            _targetLine.destroy()
        }
    }

    // Preserve QGC's normal custom map-item mechanism.
    Instantiator {
        model: QGroundControl.corePlugin.customMapItems

        Item {
            property var _customObject

            Component.onCompleted: {
                var controlUrl = object.url
                if (controlUrl !== "") {
                    var component = Qt.createComponent(controlUrl)
                    if (component.status === Component.Ready) {
                        _customObject = component.createObject(map, { "customMapObject": object })
                        if (_customObject) {
                            map.addMapItem(_customObject)
                        }
                    } else {
                        console.log("Component creation failed", component.errorString())
                    }
                }
            }

            Component.onDestruction: {
                if (_customObject) {
                    _customObject.destroy()
                }
            }
        }
    }
}
