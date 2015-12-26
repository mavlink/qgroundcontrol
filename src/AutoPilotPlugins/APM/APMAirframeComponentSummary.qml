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

    APMAirframeComponentController {
        id:         controller
        factPanel:  panel
    }

    property Fact sysIdFact:        controller.getParameterFact(-1, "FRAME")

    Grid {
        id:         grid
        rows:       1
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Frame Type:" }
        QGCLabel { text: sysIdFact.enumStringValue }
    }
}
