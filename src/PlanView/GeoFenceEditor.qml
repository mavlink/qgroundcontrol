import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

QGCFlickable {
    id:             root
    width:          availableWidth
    height:         Math.min(availableHeight, geoFenceEditorRect.height)
    contentHeight:  geoFenceEditorRect.height
    clip:           true

    property real   availableWidth
    property real   availableHeight
    property var    myGeoFenceController
    property var    flightMap

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2

    property var polygon: myGeoFenceController.polygon

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
            text:               qsTr("GeoFence")
            color:              "black"
        }

        Rectangle {
            id:                 geoFenceItems
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        geoFenceLabel.bottom
            height:             fenceColumn.y + fenceColumn.height + (_margin * 2)
            color:              qgcPal.windowShadeDark
            radius:             _radius

            Column {
                id:                 fenceColumn
                anchors.margins:    _margin
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    id:                 geoLabel
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.smallFontPointSize
                    text:               qsTr("GeoFencing allows you to set a virtual ‘fence’ around the area you want to fly in.")
                }

                Repeater {
                    model: myGeoFenceController.params

                    Item {
                        width:  fenceColumn.width
                        height: textField.height

                        property bool showCombo: modelData.enumStrings.length > 0

                        QGCLabel {
                            id:                 textFieldLabel
                            anchors.baseline:   textField.baseline
                            text:               myGeoFenceController.paramLabels[index]
                        }

                        FactTextField {
                            id:             textField
                            anchors.right:  parent.right
                            width:          _editFieldWidth
                            showUnits:      true
                            fact:           modelData
                            visible:        !parent.showCombo
                        }

                        FactComboBox {
                            id:             comboField
                            anchors.right:  parent.right
                            width:          _editFieldWidth
                            indexModel:     false
                            fact:           showCombo ? modelData : _nullFact
                            visible:        parent.showCombo

                            property var _nullFact: Fact { }
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Add fence polygon")
                    visible:    myGeoFenceController.polygonSupported && myGeoFenceController.mapPolygon.count === 0
                    onClicked:  myGeoFenceController.addPolygon()
                }

                QGCButton {
                    text:       qsTr("Remove fence polygon")
                    visible:    myGeoFenceController.polygonSupported && myGeoFenceController.mapPolygon.count > 0
                    onClicked:  myGeoFenceController.removePolygon()
                }
            }
        }
    }
}
