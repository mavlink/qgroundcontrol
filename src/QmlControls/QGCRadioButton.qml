import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

RadioButton {
    id:             control
    font.family:    ScreenTools.normalFontFamily
    font.pointSize: ScreenTools.defaultFontPointSize

    property color  textColor:  qgcPal.textPrimary
    property int    wrapMode:   Text.NoWrap
    property bool   _noText:    text === ""

    QGCPalette { id:qgcPal; colorGroupEnabled: enabled }

    indicator: Rectangle {
        implicitWidth:          ScreenTools.radioButtonIndicatorSize
        implicitHeight:         width
        color:                  control.enabled ? "white" : "transparent"
        border.color:           control.enabled ? qgcPal.textSecondary : qgcPal.borderSubtle
        radius:                 height / 2
        x:                      control.leftPadding
        y:                      parent.height / 2 - height / 2

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.accent
            opacity:        control.hovered ? .2 : 0
            radius:         parent.radius
        }

        Rectangle {
            anchors.centerIn:   parent
            // Width should be an odd number to be centralized by the parent properly
            width:              2 * Math.floor(parent.width / 4) + 1
            height:             width
            antialiasing:       true
            radius:             height * 0.5
            color:              qgcPal.accent
            visible:            control.checked
        }
    }

    contentItem: Text {
        text:               control.text
        font.family:        control.font.pointSize
        font.pointSize:     control.font.pointSize
        font.bold:          control.font.bold
        color:              control.textColor
        wrapMode:           control.wrapMode
        verticalAlignment:  Text.AlignVCenter
        leftPadding:        control.indicator.width + (_noText ? 0 : ScreenTools.defaultFontPixelWidth * 0.25)
    }

}
