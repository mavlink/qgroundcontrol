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

    property Fact returnAltFact:    controller.getParameterFact(-1, "RTL_RETURN_ALT")
    property Fact descendAltFact:   controller.getParameterFact(-1, "RTL_DESCEND_ALT")
    property Fact landDelayFact:    controller.getParameterFact(-1, "RTL_LAND_DELAY")
    property Fact commDLLossFact:   controller.getParameterFact(-1, "COM_DL_LOSS_EN")
    property Fact commRCLossFact:   controller.getParameterFact(-1, "COM_RC_LOSS_T")

    Grid {
        id:         grid
        rows:       5
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "RTL min alt:" }
        QGCLabel { text: returnAltFact.valueString }

        QGCLabel { text: "RTL home alt:" }
        QGCLabel { text: descendAltFact.valueString }

        QGCLabel { text: "RTL loiter delay:" }
        QGCLabel { text: landDelayFact.value < 0 ? "Disabled" : landDelayFact.valueString }

        QGCLabel { text: "Telemetry loss RTL:" }
        QGCLabel { text: commDLLossFact.value != -1 ? "Disabled" : commDLLossFact.valueString }

        QGCLabel { text: "RC loss RTL (seconds):" }
        QGCLabel { text: commRCLossFact.valueString }
    }
}
