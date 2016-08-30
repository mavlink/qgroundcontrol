import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

QGCFlickable {
    id:             root
    width:          availableWidth
    height:         Math.min(availableHeight, geoFenceEditorRect.height)
    contentHeight:  geoFenceEditorRect.height
    clip:           true

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    property var polygon: geoFenceController.polygon

    Connections {
        target: geoFenceController.polygon

        onPathChanged: {
            if (geoFenceController.polygon.path.length > 2) {
                geoFenceController.breachReturnPoint = geoFenceController.polygon.center()
            }
        }
    }

    Rectangle {
        id:     geoFenceEditorRect
        width:  parent.width
        height: geoFenceItems.y + geoFenceItems.height + (_margin * 2)
        radius: _radius
        color:  qgcPal.buttonHighlight

        QGCLabel {
            id:                 geoFenceLabel
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.top:        parent.top
            text:               qsTr("Geo-Fence (WIP careful!)")
            color:              "black"
        }

        Rectangle {
            id:                 geoFenceItems
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        geoFenceLabel.bottom
            height:             editorColumn.height + (_margin * 2)
            color:              qgcPal.windowShadeDark
            radius:             _radius

            Column {
                id:                 editorColumn
                anchors.margins:    _margin
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _margin

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           qsTr("Click in map to set breach return point.")
                }

                QGCLabel { text: qsTr("Fence Settings:") }

                Rectangle {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    height:         1
                    color:          qgcPal.text
                }

                QGCLabel {
                    text:       qsTr("Must be connected to Vehicle to change fence settings.")
                    visible:    !QGroundControl.multiVehicleManager.activeVehicle
                }

                Repeater {
                    model: geoFenceController.params

                    Item {
                        width:  editorColumn.width
                        height: textField.height

                        QGCLabel {
                            id:                 textFieldLabel
                            anchors.baseline:   textField.baseline
                            text:               modelData.name
                        }

                        FactTextField {
                            id:             textField
                            anchors.right:  parent.right
                            width:          _editFieldWidth
                            showUnits:      true
                            fact:           modelData
                        }
                    }
                }

                QGCMapPolygonControls {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    flightMap:      editorMap
                    polygon:        root.polygon
                    sectionLabel:   qsTr("Fence Polygon:")
                }
            }
        }
    }
}
