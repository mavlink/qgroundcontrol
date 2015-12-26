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

    property Fact _failsafeBattMah:     controller.getParameterFact(-1, "FS_BATT_MAH")
    property Fact _failsafeBattVoltage: controller.getParameterFact(-1, "FS_BATT_VOLTAGE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

    property Fact _rtlAltFact: controller.getParameterFact(-1, "ALT_HOLD_RTL")

    Grid {
        id:         grid
        rows:       4
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Throttle failsafe:" }
        QGCLabel { text: _failsafeThrEnable.value != 0 ? _failsafeThrValue.valueString : "Disabled" }

        QGCLabel { text: "Voltage failsafe:" }
        QGCLabel { text: _failsafeBattVoltage.value == 0 ? "Disabled" : _failsafeBattVoltage.valueString }

        QGCLabel { text: "mAh failsafe:" }
        QGCLabel { text: _failsafeBattMah.value == 0 ? "Disabled" : _failsafeBattMah.valueString }

        QGCLabel { text: "RTL min alt:" }
        QGCLabel { text: _rtlAltFact.value < 0 ? "current" : _rtlAltFact.valueString }
    }
}
