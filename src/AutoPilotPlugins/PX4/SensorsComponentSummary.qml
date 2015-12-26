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

    property Fact mag0IdFact:   controller.getParameterFact(-1, "CAL_MAG0_ID")
    property Fact gyro0IdFact:  controller.getParameterFact(-1, "CAL_GYRO0_ID")
    property Fact accel0IdFact: controller.getParameterFact(-1, "CAL_ACC0_ID")

    Grid {
        id:         grid
        rows:       3
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Compass:" }
        QGCLabel { text: mag0IdFact.value  == 0 ? "Setup required" : "Ready" }

        QGCLabel { text: "Gyro:" }
        QGCLabel { text: gyro0IdFact.value  == 0 ? "Setup required" : "Ready" }

        QGCLabel { text: "Accelerometer:" }
        QGCLabel { text: accel0IdFact.value  == 0 ? "Setup required" : "Ready" }
    }
}
