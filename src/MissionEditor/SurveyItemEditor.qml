import QtQuick                  2.2
import QtQuick.Controls         1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Survery mission items
Rectangle {
    id:         _root
    height:     editorColumn.height + (_margin * 2)
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierachy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property bool _addPointsMode:   false
    property real _margin:          ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        Connections {
            target: editorMap

            onMapClicked: {
                if (_addPointsMode) {
                    missionItem.addPolygonCoordinate(coordinate)
                }
            }
        }

        QGCLabel {
            text:       "Fly a grid pattern over a defined area."
            wrapMode:   Text.WordWrap
        }

        Repeater {
            model: [ missionItem.gridAltitude, missionItem.gridAngle, missionItem.gridSpacing ]

            Item {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         textField.height

                QGCLabel {
                    anchors.baseline:   textField.baseline
                    anchors.left:       parent.left
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

        QGCButton {
            text: _addPointsMode ? "Finished" : "Draw Polygon"
            onClicked: {
                if (_addPointsMode) {
                    _addPointsMode = false
                } else {
                    missionItem.clearPolygon()
                    _addPointsMode = true
                }
            }
        }
    }
}
