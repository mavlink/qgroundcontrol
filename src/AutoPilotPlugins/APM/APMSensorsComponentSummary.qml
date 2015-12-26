import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

FactPanel {
    id:     panel
    width:  grid.width
    height: grid.height
    color:  qgcPal.windowShade

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    APMSensorsComponentController { id: controller; factPanel: panel }

    property bool accelCalNeeded:   controller.accelSetupNeeded
    property bool compassCalNeeded: controller.compassSetupNeeded

    Grid {
        id:         grid
        rows:       2
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Compass:" }
        QGCLabel { text: compassCalNeeded ? "Setup required" : "Ready" }

        QGCLabel { text: "Accelerometer:" }
        QGCLabel { text: accelCalNeeded ? "Setup required" : "Ready" }
    }
}
