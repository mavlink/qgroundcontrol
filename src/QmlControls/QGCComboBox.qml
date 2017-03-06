import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ComboBox {
    property var    _qgcPal:        QGCPalette { colorGroupEnabled: enabled }
    property bool   _showHighlight: pressed | hovered
    property bool   _showBorder:    _qgcPal.globalTheme === QGCPalette.Light

    style: ComboBoxStyle {
        font.pointSize: ScreenTools.defaultFontPointSize
        textColor: _showHighlight ?
                    control._qgcPal.buttonHighlightText :
                    control._qgcPal.buttonText

        background: Item {
            implicitWidth:  ScreenTools.implicitComboBoxWidth
            implicitHeight: ScreenTools.implicitComboBoxHeight

            Rectangle {
                anchors.fill:   parent
                color:          _showHighlight ? control._qgcPal.buttonHighlight : control._qgcPal.button
                border.width:   _showBorder ? 1: 0
                border.color:  control._qgcPal.buttonText
            }

            Image {
                id: imageItem
                source: "/qmlimages/arrow-down.png"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: dropDownButtonWidth / 2
                opacity: control.enabled ? 0.6 : 0.3
            }
        }
    }

    // Capture Wheel events to disable scrolling options in ComboBox.
    // As a side effect, this also prevents scrolling the page when
    // mouse is over a ComboBox, but this would also the case when
    // scrolling items in the ComboBox is enabled.
    MouseArea {
        anchors.fill: parent
        onWheel: {
            // do nothing
            wheel.accepted = true;
        }
        onPressed: {
            // propogate to ComboBox
            mouse.accepted = false;
        }
        onReleased: {
            // propogate to ComboBox
            mouse.accepted = false;
        }
    }
}
