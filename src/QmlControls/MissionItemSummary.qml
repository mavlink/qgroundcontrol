import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0

/// Mission item summary display control
Rectangle {
    property var missionItem        ///< Mission Item object

    width:          ScreenTools.defaultFontPixelWidth * 15
    height:         valueColumn.height + radius
    border.width:   2
    border.color:   "white"
    color:          "white"
    opacity:        0.75
    radius:         ScreenTools.defaultFontPixelWidth

    MissionItemIndexLabel {
        anchors.top:        parent.top
        anchors.right:      parent.right
        missionItemIndex:   missionItem.id + 1
    }

    Column {
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top

        QGCLabel {
            color:                  "black"
            horizontalAlignment:    Text.AlignTop
            font.weight:            Font.Bold
            text:                   missionItem.commandName
        }

        Repeater {
            model: missionItem.valueLabels

            QGCLabel {
                color:  "black"
                text:   modelData
            }
        }
    }

    Column {
        id:                 valueColumn
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top

        QGCLabel {
            font.weight:    Font.Bold
            text:           " "
        }

        Repeater {
            model: missionItem.valueStrings

            QGCLabel {
                width:                  valueColumn.width
                color:                  "black"
                text:                   modelData
                horizontalAlignment:    Text.AlignRight
            }
        }
    }
}
