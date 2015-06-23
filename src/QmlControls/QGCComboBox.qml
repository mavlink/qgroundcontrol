import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

ComboBox {
    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }
    property bool __showHighlight: pressed | hovered

    style: ComboBoxStyle {
        font.pixelSize: ScreenTools.defaultFontPixelSize
        textColor: __showHighlight ?
                    control.__qgcPal.buttonHighlightText :
                    control.__qgcPal.buttonText

        background: Item {
            implicitWidth: Math.round(TextSingleton.implicitHeight * 4.5)
            implicitHeight: Math.max(25, Math.round(TextSingleton.implicitHeight * 1.2))

            Rectangle {
                anchors.fill: parent
                color: __showHighlight ?
                    control.__qgcPal.buttonHighlight :
                    control.__qgcPal.button
            }

            Image {
                id: imageItem
                visible: control.menu !== null
                source: "/qmlimages/arrow-down.png"
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: dropDownButtonWidth / 2
                opacity: control.enabled ? 0.6 : 0.3
            }
        }
    }
}
