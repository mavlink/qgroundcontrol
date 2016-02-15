import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Vehicle       1.0

// This control is used when the activeVehicle on the Fly view is clicked. It gives the user
// the options for taking off.
Item {
    id: root

    property var vehicle

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Slider {
        id:             slider
        orientation:    Qt.Vertical
        minimumValue:   0
        maximumValue:   100
        stepSize:       1
        value:          10
        anchors.verticalCenter: parent.verticalCenter
    }

    Column {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       slider.right
        spacing:            ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: slider.verticalCenter

        QGCButton {
            text:               "Takeoff to " + slider.value + " meters"

            onClicked: {
                root.visible = false
                vehicle.takeoff(slider.value)
            }
        }

        QGCButton {
            text:       "Cancel"
            onClicked:  root.visible= false
        }
    }
}
