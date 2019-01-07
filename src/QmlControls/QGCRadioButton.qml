import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

RadioButton {
    property color  textColor:          _qgcPal.text
    property bool   textBold:           false
    property real   textFontPointSize:  ScreenTools.defaultFontPointSize

    property var    _qgcPal:             QGCPalette { colorGroupEnabled: enabled }

    property bool _noText: text === ""

    activeFocusOnPress: true

    style: RadioButtonStyle {
        spacing: _noText ? 0 : ScreenTools.defaultFontPixelWidth / 2

        label: Item {
            implicitWidth:          _noText ? 0 : text.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.25
            implicitHeight:         _noText ? 0 : Math.max(text.implicitHeight, ScreenTools.radioButtonIndicatorSize)
            baselineOffset:         text.y + text.baselineOffset

            Text {
                id:                 text
                text:               control.text
                font.pointSize:     textFontPointSize
                font.bold:          control.textBold
                color:              control.textColor
                anchors.centerIn:   parent
            }
        }

        indicator: Rectangle {
            width:          ScreenTools.radioButtonIndicatorSize
            height:         width
            color:          "white"
            border.color:   "black"
            radius:         height / 2
            opacity:        control.enabled ? 1 : 0.5

            Rectangle {
                anchors.centerIn:   parent
                width:              Math.round(parent.width * 0.5)
                height:             width
                antialiasing:       true
                radius:             height / 2
                color:              "black"
                visible:            control.checked
            }
        }
    }
}
