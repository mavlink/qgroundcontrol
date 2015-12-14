import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

RadioButton {
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    style: RadioButtonStyle {
        label: Item {
            implicitWidth: text.implicitWidth + 2
            implicitHeight: text.implicitHeight
            baselineOffset: text.y + text.baselineOffset
            Rectangle {
                anchors.fill: text
                anchors.margins: -1
                anchors.leftMargin: -3
                anchors.rightMargin: -3
                visible: control.activeFocus
                height: 6
                radius: 3
                color: "#224f9fef"
                border.color: "#47b"
                opacity: 0.6
            }
            Text {
                id:             text
                text:           control.text
                font.pixelSize: ScreenTools.defaultFontPixelSize
                antialiasing:   true

                anchors.centerIn: parent

                color: control.__qgcPal.text
            }
        }
    }
}
