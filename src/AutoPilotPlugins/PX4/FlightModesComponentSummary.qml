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

    property Fact modeSwFact:   controller.getParameterFact(-1, "RC_MAP_MODE_SW")
    property Fact posCtlSwFact: controller.getParameterFact(-1, "RC_MAP_POSCTL_SW")
    property Fact loiterSwFact: controller.getParameterFact(-1, "RC_MAP_LOITER_SW")
    property Fact returnSwFact: controller.getParameterFact(-1, "RC_MAP_RETURN_SW")

    Grid {
        id:         grid
        rows:       4
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Mode switch:" }
        QGCLabel { text: modeSwFact.value == 0 ? "Setup required" : modeSwFact.valueString }

        QGCLabel { text: "Position Ctl switch:" }
        QGCLabel { text: posCtlSwFact.value == 0 ? "Disabled" : posCtlSwFact.valueString }

        QGCLabel { text: "Loiter switch:" }
        QGCLabel { text: loiterSwFact.value == 0 ? "Disabled" : loiterSwFact.valueString }

        QGCLabel { text: "Return switch:" }
        QGCLabel { text: returnSwFact.value == 0 ? "Disabled" : returnSwFact.valueString }
    }
}
