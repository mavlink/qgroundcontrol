pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Wrapping chips for RTCM message counts given as [messageId, count] pairs; ID 0 is unidentified.
Flow {
    id: root

    property var messageCounts: []

    spacing: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }

    Repeater {
        model: root.messageCounts

        delegate: Rectangle {
            id: chip

            required property var modelData

            implicitWidth:  chipLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.2
            implicitHeight: chipLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.3
            radius:         implicitHeight / 2
            color:          qgcPal.windowShade
            border.color:   qgcPal.groupBorder
            border.width:   1

            QGCLabel {
                id: chipLabel

                anchors.centerIn: parent
                //: %1 is an RTCM message ID, %2 is the number of messages received with that ID
                text:             qsTr("%1 × %2").arg(chip.modelData[0] === 0 ? qsTr("unknown") : chip.modelData[0])
                                                 .arg(chip.modelData[1])
                textFormat:       Text.PlainText
                font.pointSize:   ScreenTools.smallFontPointSize
            }
        }
    }
}
