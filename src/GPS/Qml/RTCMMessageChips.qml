pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Wrapping chips for RTCM message counts; ID 0 is unidentified. A chip stays while its count changes.
Flow {
    id: root

    property RTCMMessageCountModel messageCounts
    /// The number of chips.
    readonly property int count: chips.count

    spacing: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }

    Repeater {
        id: chips

        model: root.messageCounts

        delegate: Rectangle {
            id: chip

            required property int messageId
            required property real count

            implicitWidth:  chipLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.2
            implicitHeight: chipLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.3
            radius:         implicitHeight / 2
            color:          qgcPal.windowCard
            border.color:   qgcPal.borderSubtle
            border.width:   1

            QGCLabel {
                id: chipLabel

                anchors.centerIn: parent
                //: %1 is an RTCM message ID, %2 is the number of messages received with that ID
                text:             qsTr("%1 × %2").arg(chip.messageId === 0 ? qsTr("unknown") : chip.messageId)
                                                 .arg(chip.count.toFixed(0))
                textFormat:       Text.PlainText
                font.pointSize:   ScreenTools.smallFontPointSize
            }
        }
    }
}
