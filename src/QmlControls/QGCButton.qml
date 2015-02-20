import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0

Button {
    // primary: true - this is the primary button for this group of buttons
    property bool primary: false

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    property bool __showHighlight: pressed | hovered | checked

    style: ButtonStyle {
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 25
                color: __showHighlight ?
                    control.__qgcPal.buttonHighlight :
                    (primary ? control.__qgcPal.primaryButton : control.__qgcPal.button)
            }

            label: Text {
                width: parent.width
                height: parent.height

                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter

                text: control.text
                color: __showHighlight ?
                    control.__qgcPal.buttonHighlightText :
                    (primary ? control.__qgcPal.primaryButtonText : control.__qgcPal.buttonText)
            }
        }
}
