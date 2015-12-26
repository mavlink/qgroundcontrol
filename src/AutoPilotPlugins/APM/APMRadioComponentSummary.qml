import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FactPanel {
    id:     panel
    width:  grid.width
    height: grid.height
    color:  qgcPal.windowShade

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact mapRollFact:      controller.getParameterFact(-1, "RCMAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RCMAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RCMAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RCMAP_THROTTLE")

    Grid {
        id:         grid
        rows:       4
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Roll:" }
        QGCLabel { text: mapRollFact.value == 0 ? "Setup required" : "Channel " + mapRollFact.valueString }

        QGCLabel { text: "Pitch:" }
        QGCLabel { text: mapPitchFact.value == 0 ? "Setup required" : "Channel " + mapPitchFact.valueString }

        QGCLabel { text: "Yaw:" }
        QGCLabel { text: mapYawFact.value == 0 ? "Setup required" : "Channel " + mapYawFact.valueString }

        QGCLabel { text: "Throttle:" }
        QGCLabel { text: mapThrottleFact.value == 0 ? "Setup required" : "Channel " + mapThrottleFact.valueString }
    }
}
