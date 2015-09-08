import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

/// Mission item summary display control
Rectangle {
    property var    missionItem                         ///< Mission Item object

    width:          ScreenTools.defaultFontPixelWidth * 15
    height:         valueColumn.height + radius
    border.width:   2
    border.color:   "white"
    color:          "white"
    radius:         ScreenTools.defaultFontPixelWidth

    MissionItemIndexLabel {
        id:                 _indexLabel
        anchors.top:        parent.top
        anchors.right:      parent.right
        isCurrentItem:      missionItem.isCurrentItem
        label:              missionItem.sequenceNumber
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
