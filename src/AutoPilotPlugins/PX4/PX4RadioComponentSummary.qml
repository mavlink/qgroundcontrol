import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FactPanel {
    id:     panel
    width:  grid.width
    height: grid.height
    color:  qgcPal.windowShade

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact mapRollFact:      controller.getParameterFact(-1, "RC_MAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RC_MAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RC_MAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RC_MAP_THROTTLE")
    property Fact mapFlapsFact:     controller.getParameterFact(-1, "RC_MAP_FLAPS")
    property Fact mapAux1Fact:      controller.getParameterFact(-1, "RC_MAP_AUX1")
    property Fact mapAux2Fact:      controller.getParameterFact(-1, "RC_MAP_AUX2")

    Grid {
        id:         grid
        rows:       7
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Roll:" }
        QGCLabel { text: mapRollFact.value == 0 ? "Setup required" : mapRollFact.valueString }

        QGCLabel { text: "Pitch:" }
        QGCLabel { text: mapPitchFact.value == 0 ? "Setup required" : mapPitchFact.valueString }

        QGCLabel { text: "Yaw:" }
        QGCLabel { text: mapYawFact.value == 0 ? "Setup required" : mapYawFact.valueString }

        QGCLabel { text: "Throttle:" }
        QGCLabel { text: mapThrottleFact.value == 0 ? "Setup required" : mapThrottleFact.valueString }

        QGCLabel { text: "Flaps:" }
        QGCLabel { text: mapFlapsFact.value == 0 ? "Disabled" : mapFlapsFact.valueString }

        QGCLabel { text: "Aux1:" }
        QGCLabel { text: mapAux1Fact.value == 0 ? "Disabled" : mapAux1Fact.valueString }

        QGCLabel { text: "Aux2:" }
        QGCLabel { text: mapAux2Fact.value == 0 ? "Disabled" : mapAux2Fact.valueString }
    }
}
