import QtQuick
import QtQuick.Controls
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

CheckBox {
    id:             control
    spacing:        _noText ? 0 : ScreenTools.defaultFontPixelWidth * 0.25
    focusPolicy:    Qt.ClickFocus

    property color  textColor:          _qgcPal.text
    property bool   textBold:           false
    property real   textFontPointSize:  ScreenTools.defaultFontPointSize

    property var    _qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   _noText: text === ""

    property ButtonGroup buttonGroup: null
    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(control)
        }
    }

    contentItem: Item {
        implicitWidth:  _noText ? 0 : text.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.25
        implicitHeight: _noText ? 0 : Math.max(text.implicitHeight, ScreenTools.checkBoxIndicatorSize)
        baselineOffset: text.baselineOffset

        Text {
            id:                 text
            text:               control.text
            font.pointSize:     textFontPointSize
            font.bold:          control.textBold
            font.family:        ScreenTools.normalFontFamily
            color:              control.textColor
            anchors.centerIn:   parent
        }
    }

    indicator:  Item {
        implicitWidth:  ScreenTools.checkBoxIndicatorSize
        implicitHeight: implicitWidth
        Rectangle {
            anchors.fill:   parent
            color:          control.enabled ? "white" : _qgcPal.text
            border.color:   _qgcPal.text
            border.width:   1
            opacity:        control.checkedState === Qt.PartiallyChecked ? 0.5 : 1
            QGCColoredImage {
                source:     "/qmlimages/checkbox-check.svg"
                color:      "black"
                opacity:    control.checkedState === Qt.Checked ? (control.enabled ? 1 : 0.5) : 0
                mipmap:     true
                fillMode:   Image.PreserveAspectFit
                width:      parent.width * 0.75
                height:     width
                sourceSize.height: height
                anchors.centerIn:  parent
            }
        }
    }
}
