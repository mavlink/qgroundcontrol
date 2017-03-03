import QtQuick          2.7
import QtQuick.Controls 2.1

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FactSystem    1.0

Column {
    id:         editorColumn
    width:      availableWidth
    spacing:    _margin

    //property real availableWidth - Available width for control - Must be passed in from Loader

    readonly property real _margin:         ScreenTools.defaultFontPixelWidth / 2
    readonly property real _editFieldWidth: Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)

    QGCLabel {
        anchors.left:   parent.left
        anchors.right:  parent.right
        wrapMode:       Text.WordWrap
        text:           qsTr("Click in map to set breach return point.")
        visible:        geoFenceController.breachReturnSupported
    }

    QGCLabel { text: qsTr("Fence Settings:") }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         1
        color:          qgcPal.text
    }

    Repeater {
        model: geoFenceController.params

        Item {
            width:  editorColumn.width
            height: textField.height

            property bool showCombo: modelData.enumStrings.length > 0

            QGCLabel {
                id:                 textFieldLabel
                anchors.baseline:   textField.baseline
                text:               geoFenceController.paramLabels[index]
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

    QGCMapPolygonControls {
        anchors.left:   parent.left
        anchors.right:  parent.right
        flightMap:      editorMap
        polygon:        geoFenceController.polygon
        sectionLabel:   qsTr("Fence Polygon:")
        visible:        geoFenceController.polygonSupported

        onPolygonEditCompleted: geoFenceController.validateBreachReturn()
    }
}
