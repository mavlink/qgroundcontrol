import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VehicleSetup
import QGroundControl.FactControls

Flow {
    spacing: ScreenTools.defaultFontPixelWidth

    property var _joystick: joystickManager.activeJoystick

    QGCPalette { id: qgcPal }

    Connections {
        target: _joystick

        onRawButtonPressedChanged: (index, pressed) => {
            if (buttonRepeater.itemAt(index)) {
                buttonRepeater.itemAt(index).pressed = pressed
            }
        }
    }

    Repeater {
        id: buttonRepeater
        model: _joystick.buttonCount

        Rectangle {
            implicitWidth: ScreenTools.defaultFontPixelHeight * 1.5
            implicitHeight: width
            border.width: 1
            border.color: qgcPal.text
            color: pressed ? qgcPal.buttonHighlight : qgcPal.button

            property bool pressed

            QGCLabel {
                anchors.fill: parent
                color: pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData
            }
        }
    }
}
