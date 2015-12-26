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

    property Fact flightMode1: controller.getParameterFact(-1, "FLTMODE1")
    property Fact flightMode2: controller.getParameterFact(-1, "FLTMODE2")
    property Fact flightMode3: controller.getParameterFact(-1, "FLTMODE3")
    property Fact flightMode4: controller.getParameterFact(-1, "FLTMODE4")
    property Fact flightMode5: controller.getParameterFact(-1, "FLTMODE5")
    property Fact flightMode6: controller.getParameterFact(-1, "FLTMODE6")

    Grid {
        id:         grid
        rows:       6
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Flight Mode 1:" }
        QGCLabel { text: flightMode1.enumStringValue }

        QGCLabel { text: "Flight Mode 2:" }
        QGCLabel { text: flightMode2.enumStringValue }

        QGCLabel { text: "Flight Mode 3:" }
        QGCLabel { text: flightMode3.enumStringValue }

        QGCLabel { text: "Flight Mode 4:" }
        QGCLabel { text: flightMode4.enumStringValue }

        QGCLabel { text: "Flight Mode 5:" }
        QGCLabel { text: flightMode5.enumStringValue }

        QGCLabel { text: "Flight Mode 6:" }
        QGCLabel { text: flightMode6.enumStringValue }
    }
}
