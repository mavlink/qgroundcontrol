import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

RowLayout {
    property alias label:                   label.text
    property alias fact:                    _comboBox.fact
    property alias indexModel:              _comboBox.indexModel
    property var   comboBox:                _comboBox
    property real  comboBoxPreferredWidth:  -1

    spacing: ScreenTools.defaultFontPixelWidth * 2

    signal activated(int index)

    Row {
        Layout.fillWidth:   true
        spacing:            0

        property bool _hasHelp: _comboBox.fact && _comboBox.fact.longDescription

        QGCLabel {
            id:                 label
        }

        Text {
            id:                     helpIndicator
            text:                   " ?"
            font.pointSize:         ScreenTools.smallFontPointSize
            font.bold:              true
            color:                  qgcPal.textLink
            visible:                parent._hasHelp
            anchors.bottom:         label.verticalCenter
            anchors.bottomMargin:   -2

            QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

            MouseArea {
                anchors.fill:       parent
                anchors.margins:    -4
                hoverEnabled:       true
                onContainsMouseChanged: {
                    if (containsMouse) {
                        toolTip.visible = true
                    } else {
                        toolTip.visible = false
                    }
                }
            }

            ToolTip {
                id:             toolTip
                text:           _comboBox.fact ? _comboBox.fact.longDescription : ""
                delay:          300
                timeout:        10000
            }
        }
    }

    FactComboBox {
        id:                     _comboBox
        Layout.preferredWidth:  comboBoxPreferredWidth
        sizeToContents:         true

        onActivated: (index) => { parent.activated(index) }
    }
}
