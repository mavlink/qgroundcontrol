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

    /// "1077 × 120, every 1 s": the ID, its count and, once known, how often it arrives.
    function _chipText(messageId, count, interval) {
        const id = messageId === 0 ? qsTr("unknown") : messageId
        const rate = root._rateText(interval)
        if (rate === "") {
            //: %1 is an RTCM message ID, %2 is the number of messages received with that ID
            return qsTr("%1 × %2").arg(id).arg(count.toFixed(0))
        }
        //: %1 is an RTCM message ID, %2 the number of messages received with that ID, %3 how often they arrive
        return qsTr("%1 × %2, %3").arg(id).arg(count.toFixed(0)).arg(rate)
    }

    /// Whole messages per second from two a second, else whole seconds between messages; "" for an unknown interval.
    function _rateText(interval) {
        if (interval <= 0) {
            return ""
        }
        const perSecond = Math.round(1 / interval)
        if (perSecond >= 2) {
            //: %1 is the number of RTCM messages of one type received per second
            return qsTr("%1 Hz").arg(perSecond)
        }
        //: %1 is the number of seconds between two RTCM messages of one type
        return qsTr("every %1 s").arg(Math.max(1, Math.round(interval)))
    }

    spacing: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }

    Repeater {
        id: chips

        model: root.messageCounts

        delegate: Rectangle {
            id: chip

            required property int messageId
            required property real count
            required property real interval

            implicitWidth:  chipLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.2
            implicitHeight: chipLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.3
            radius:         implicitHeight / 2
            color:          qgcPal.windowCard
            border.color:   qgcPal.borderSubtle
            border.width:   1

            QGCLabel {
                id: chipLabel

                anchors.centerIn: parent
                text:             root._chipText(chip.messageId, chip.count, chip.interval)
                textFormat:       Text.PlainText
                font.pointSize:   ScreenTools.smallFontPointSize
            }
        }
    }
}
