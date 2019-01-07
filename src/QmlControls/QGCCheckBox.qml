import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

CheckBox {
    property color  textColor:          _qgcPal.text
    property bool   textBold:           false
    property real   textFontPointSize:  ScreenTools.defaultFontPointSize

    property var    _qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool   _noText: text === ""
    property real   _radius: ScreenTools.defaultFontPixelHeight * 0.16

    activeFocusOnPress: true

    style: CheckBoxStyle {
        label: Item {
            implicitWidth:  _noText ? 0 : text.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.25
            implicitHeight: _noText ? 0 : Math.max(text.implicitHeight, ScreenTools.checkBoxIndicatorSize)
            baselineOffset: text.baselineOffset

            Text {
                id:                 text
                text:               control.text
                font.pointSize:     textFontPointSize
                font.bold:          control.textBold
                color:              control.textColor
                anchors.centerIn:   parent
            }
        }

        indicator:  Item {
            implicitWidth:  ScreenTools.checkBoxIndicatorSize
            implicitHeight: implicitWidth

            Rectangle {
                anchors.fill:   parent
                radius:         _radius
                border.color:   "black"
                opacity:        control.checkedState === Qt.PartiallyChecked ? 0.5 : 1

                Rectangle {
                    anchors.margins:    parent.height / 4
                    anchors.fill:       parent
                    radius:             _radius
                    color:              "black"
                    visible:            control.checkedState === Qt.Checked
                }
            }
        }
    }
}
