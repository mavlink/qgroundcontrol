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

    property Fact _failsafeBattEnable:  controller.getParameterFact(-1, "FS_BATT_ENABLE")
    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")

    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")
    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")

    property string _failsafeBattEnableText
    property string _failsafeThrEnableText

    Component.onCompleted: {
        setFailsafeBattEnableText()
        setFailsafeThrEnableText()
    }

    Connections {
        target: _failsafeBattEnable

        onValueChanged: setFailsafeBattEnableText()
    }

    Connections {
        target: _failsafeThrEnable

        onValueChanged: setFailsafeThrEnableText()
    }

    function setFailsafeThrEnableText() {
        switch (_failsafeThrEnable.value) {
        case 0:
            _failsafeThrEnableText = "Disabled"
            break
        case 1:
            _failsafeThrEnableText = "Always RTL"
            break
        case 2:
            _failsafeThrEnableText = "Continue with Mission in Auto Mode"
            break
        case 3:
            _failsafeThrEnableText = "Always Land"
            break
        default:
            _failsafeThrEnableText = "Unknown"
        }
    }

    function setFailsafeBattEnableText() {
        switch (_failsafeBattEnable.value) {
        case 0:
            _failsafeBattEnableText = "Disabled"
            break
        case 1:
            _failsafeBattEnableText = "Land"
            break
        case 2:
            _failsafeBattEnableText = "Return to Launch"
            break
        default:
            _failsafeThrEnableText = "Unknown"
        }
    }

    Grid {
        id:         grid
        rows:       8
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Throttle failsafe:" }
        QGCLabel { text: _failsafeThrEnableText }

        QGCLabel { text: "Battery failsafe:" }
        QGCLabel { text: _failsafeBattEnableText }

        QGCLabel { text: "GeoFence:" }
        QGCLabel { text: _fenceEnable.value == 0 || _fenceType == 0 ?
                             "Disabled" :
                             (_fenceType.value == 1 ?
                                  "Altitude" :
                                  (_fenceType.value == 2 ? "Circle" : "Altitude,Circle")) }

        QGCLabel { text: "GeoFence:"; visible: _fenceEnable.value != 0 }
        QGCLabel { text: _fenceAction.value == 0 ?
                             "Report only" :
                             (_fenceAction.value == 1 ? "RTL or Land" : "Unknown")
            visible: _fenceEnable.value != 0
        }

        QGCLabel { text: "RTL min alt:" }
        QGCLabel { text: _rtlAltFact.value == 0 ? "current" : _rtlAltFact.valueString }

        QGCLabel { text: "RTL loiter time:" }
        QGCLabel { text: _rtlLoitTimeFact.valueString }

        QGCLabel { text: "RTL final alt:" }
        QGCLabel { text: _rtlAltFinalFact.value == 0 ? "Land" : _rtlAltFinalFact.valueString }

        QGCLabel { text: "Descent speed:" }
        QGCLabel { text: _landSpeedFact.valueString }
    }
}
