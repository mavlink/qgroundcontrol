import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

RadioButton {
    property var    color:          qgcPal.text    ///< Text color
    property int    textStyle:      Text.Normal
    property color  textStyleColor: qgcPal.text
    property bool   textBold:       false
    property var    qgcPal:         QGCPalette { colorGroupEnabled: enabled }

    style: RadioButtonStyle {
        label: Item {
            implicitWidth:          text.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.25
            implicitHeight:         ScreenTools.implicitRadioButtonHeight
            baselineOffset:         text.y + text.baselineOffset

            Rectangle {
                anchors.fill:       text
                anchors.margins:    -1
                anchors.leftMargin: -3
                anchors.rightMargin:-3
                visible:            control.activeFocus
                height:             ScreenTools.defaultFontPixelWidth * 0.25
                radius:             height * 0.5
                color:              "#224f9fef"
                border.color:       "#47b"
                opacity:            0.6
            }

            Text {
                id:                 text
                text:               control.text
                font.pointSize:     ScreenTools.defaultFontPointSize
                font.family:        ScreenTools.normalFontFamily
                font.bold:          control.textBold
                antialiasing:       true
                color:              control.color
                style:              control.textStyle
                styleColor:         control.textStyleColor
                anchors.centerIn:   parent
            }
        }

        indicator: Rectangle {
            width:          ScreenTools.radioButtonIndicatorSize
            height:         width
            color:          "white"
            border.color:   control.qgcPal.text
            antialiasing:   true
            radius:         height / 2

            Rectangle {
                anchors.centerIn:   parent
                width:              Math.round(parent.width * 0.5)
                height:             width
                antialiasing:       true
                radius:             height / 2
                color:              "black"
                opacity:            control.checked ? (control.enabled ? 1 : 0.5) : 0
            }
        }
    }
}
