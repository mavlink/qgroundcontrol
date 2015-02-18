import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0

Button {
    property var __qgcPal: QGCPalette { colorGroup: enabled ? QGCPalette.Active : QGCPalette.Disabled }

    style: ButtonStyle {
            background: Rectangle {
                implicitWidth: 100
                implicitHeight: 25
                color: control.pressed ? control.__qgcPal.buttonHighlight : control.__qgcPal.button
            }

            label: Text {
                width: parent.width
                height: parent.height

                verticalAlignment: TextEdit.AlignVCenter
                horizontalAlignment: TextEdit.AlignHCenter

                text: control.text
                color: control.__qgcPal.buttonText
            }
        }
}
