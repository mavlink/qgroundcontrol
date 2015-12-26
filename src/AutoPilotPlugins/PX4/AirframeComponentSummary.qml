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
    AirframeComponentController { id: controller; factPanel: panel }

    property Fact sysIdFact:        controller.getParameterFact(-1, "MAV_SYS_ID")
    property Fact sysAutoStartFact: controller.getParameterFact(-1, "SYS_AUTOSTART")

    property bool autoStartSet: sysAutoStartFact.value != 0

    Grid {
        id:         grid
        rows:       3
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "System ID:" }
        FactLabel { fact: sysIdFact }

        QGCLabel { text: "Airframe type:" }
        QGCLabel { text: autoStartSet ? controller.currentAirframeType : "Setup required" }

        QGCLabel { text: "Vehicle:" }
        QGCLabel { text: autoStartSet ? controller.currentVehicleName : "Setup required" }
    }
}
