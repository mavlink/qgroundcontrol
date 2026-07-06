import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

RadioButton {
    id:             control
    font.family:    ScreenTools.normalFontFamily
    font.pointSize: ScreenTools.defaultFontPointSize

    property color  textColor:  _qgcPal.text
    property var    _qgcPal:    QGCPalette { colorGroupEnabled: enabled }
    property bool   _noText:    text === ""

    indicator: Rectangle {
        implicitWidth:          ScreenTools.radioButtonIndicatorSize
        implicitHeight:         width
        color:                  control.checked ? Qt.rgba(_qgcPal.primaryButton.r, _qgcPal.primaryButton.g, _qgcPal.primaryButton.b, 0.18) : Qt.rgba(1, 1, 1, 0.055)
        border.color:           control.checked ? _qgcPal.primaryButton : Qt.rgba(0.82, 0.88, 0.94, 0.34)
        border.width:           1
        radius:                 height / 2
        opacity:                control.enabled ? 1 : 0.5
        x:                      control.leftPadding
        y:                      parent.height / 2 - height / 2
        Rectangle {
            anchors.centerIn:   parent
            // Width should be an odd number to be centralized by the parent properly
            width:              2 * Math.floor(parent.width / 4) + 1
            height:             width
            antialiasing:       true
            radius:             height * 0.5
            color:              _qgcPal.primaryButton
            visible:            control.checked
        }
    }

    contentItem: Text {
        text:               control.text
        font.family:        control.font.family
        font.pointSize:     control.font.pointSize
        font.bold:          control.font.bold
        color:              control.textColor
        opacity:            enabled ? 1.0 : 0.3
        verticalAlignment:  Text.AlignVCenter
        leftPadding:        control.indicator.width + (_noText ? 0 : ScreenTools.defaultFontPixelWidth * 0.25)
    }

}
