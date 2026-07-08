import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

CheckBox {
    id:             control
    spacing:        _noText ? 0 : ScreenTools.defaultFontPixelWidth
    focusPolicy:    Qt.ClickFocus

    property color  textColor:          _qgcPal.text
    property bool   textBold:           false
    property real   textFontPointSize:  ScreenTools.controlFontPointSize

    property var    _qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   _noText: text === ""

    property ButtonGroup buttonGroup: null
    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(control)
        }
    }

    contentItem: Text {
        //implicitWidth:  _noText ? 0 : text.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.25
        //implicitHeight: _noText ? 0 : Math.max(text.implicitHeight, ScreenTools.checkBoxIndicatorSize)
        leftPadding:        control.indicator.width + control.spacing
        verticalAlignment:  Text.AlignVCenter
        text:               control.text
        font.pointSize:     textFontPointSize
        font.bold:          control.textBold
        font.family:        ScreenTools.normalFontFamily
        color:              control.textColor
    }

    indicator:  Rectangle {
        implicitWidth:  ScreenTools.implicitCheckBoxHeight
        implicitHeight: implicitWidth
        x:              control.leftPadding
        y:              parent.height / 2 - height / 2
        color:          control.checked ? _qgcPal.primaryButton : Qt.rgba(1, 1, 1, 0.055)
        border.color:   control.checked ? _qgcPal.primaryButton : Qt.rgba(0.82, 0.88, 0.94, 0.34)
        border.width:   1
        radius:         Math.round(ScreenTools.buttonBorderRadius * 1.2)
        opacity:        control.checkState === Qt.PartiallyChecked ? 0.5 : 1

        QGCColoredImage {
            source:             "/qmlimages/checkbox-check.svg"
            color:              _qgcPal.primaryButtonText
            mipmap:             true
            fillMode:           Image.PreserveAspectFit
            width:              parent.implicitWidth * 0.75
            height:             width
            sourceSize.height:  height
            visible:            control.checked
            anchors.centerIn:   parent
        }
    }
}
