import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ComboBox {
    property var    _qgcPal:        QGCPalette { colorGroupEnabled: enabled }
    property bool   _showHighlight: pressed | hovered
    property bool   _showBorder:    _qgcPal.globalTheme == QGCPalette.Light

    style: ComboBoxStyle {
        font.pixelSize: ScreenTools.defaultFontPixelSize
        textColor: _showHighlight ?
                    control._qgcPal.buttonHighlightText :
                    control._qgcPal.buttonText

        background: Item {
            implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
            implicitHeight: ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 3 * 0.75 : Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))

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
}
