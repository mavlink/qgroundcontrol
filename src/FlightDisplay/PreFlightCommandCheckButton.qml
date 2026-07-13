/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

PreFlightCheckButton {
    id: root

    property bool commandAvailable: true

    readonly property real _sendButtonWidth:  Math.max(ScreenTools.minTouchPixels * 1.25, ScreenTools.defaultFontPixelWidth * 7.5)
    readonly property real _sendButtonHeight: Math.max(ScreenTools.minTouchPixels * 0.78, ScreenTools.defaultFontPixelHeight * 1.7)

    signal commandRequested()

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    contentItem: RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        ColumnLayout {
            Layout.fillWidth: true
            spacing:          Math.round(ScreenTools.defaultFontPixelHeight * 0.25)

            QGCLabel {
                Layout.fillWidth:    true
                text:                root.name
                font.bold:           true
                font.pointSize:      ScreenTools.defaultFontPointSize
                wrapMode:            Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                color:               qgcPal.buttonText
            }

            QGCLabel {
                Layout.fillWidth:    true
                text:                root.manualText
                font.pointSize:      ScreenTools.smallFontPointSize
                wrapMode:            Text.WordWrap
                horizontalAlignment: Text.AlignLeft
                color:               qgcPal.buttonText
                opacity:             0.72
            }
        }

        QGCButton {
            id:                     sendButton
            Layout.alignment:       Qt.AlignRight | Qt.AlignVCenter
            Layout.rightMargin:     ScreenTools.defaultFontPixelWidth * 0.75
            Layout.minimumWidth:    root._sendButtonWidth
            Layout.preferredWidth:  root._sendButtonWidth
            Layout.maximumWidth:    root._sendButtonWidth
            Layout.minimumHeight:   root._sendButtonHeight
            Layout.preferredHeight: root._sendButtonHeight
            Layout.maximumHeight:   root._sendButtonHeight
            topPadding:             0
            bottomPadding:          0
            leftPadding:            ScreenTools.defaultFontPixelWidth
            rightPadding:           ScreenTools.defaultFontPixelWidth
            heightFactor:           0.18
            pointSize:              ScreenTools.smallFontPointSize
            backRadius:             Math.round(ScreenTools.defaultFontPixelWidth * 0.45)
            text:                   qsTr("Send")
            enabled:                root.enabled && root.commandAvailable
            onClicked:              root.commandRequested()

            contentItem: Item {
                QGCLabel {
                    anchors.centerIn: parent
                    text:             sendButton.text
                    font.pointSize:   ScreenTools.smallFontPointSize
                    color:            sendButton.pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                }
            }
        }
    }
}
