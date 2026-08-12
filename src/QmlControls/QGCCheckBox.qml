import QtQuick
import QtQuick.Controls

import QGroundControl.Palette
import QGroundControl.ScreenTools

CheckBox {
    id: control
    spacing: _noText ? 0 : 10
    focusPolicy: Qt.ClickFocus

    property color  textColor:          _qgcPal.text
    property bool   textBold:           false
    property real   textFontPointSize:  11

    property var    _qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   _noText: text === ""

    property ButtonGroup buttonGroup: null
    onButtonGroupChanged: {
        if (buttonGroup) {
            buttonGroup.addButton(control)
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        text: control.text
        font.pixelSize: 14
        font.weight: control.textBold ? Font.Bold : Font.Normal
        font.family: ScreenTools.normalFontFamily
        color: control.textColor
    }

    indicator: Rectangle {
        implicitWidth: 20
        implicitHeight: 20
        y: parent.height / 2 - height / 2
        color: control.checked ? _qgcPal.primaryButton : "transparent"
        border.color: control.checked ? _qgcPal.primaryButton : _qgcPal.buttonBorder
        border.width: 1
        radius: 6

        Behavior on color { ColorAnimation { duration: 80 } }
        Behavior on border.color { ColorAnimation { duration: 80 } }

        Text {
            anchors.centerIn: parent
            text: "✓"
            font.pixelSize: 13
            font.weight: Font.Bold
            color: _qgcPal.primaryButtonText
            visible: control.checked
        }
    }
}
