import QtQuick

import QGroundControl

/// Manages access to ArduPilot battery parameters for a given battery index.
/// ArduPilot battery parameter prefixes: BATT_, BATT2_, ..., BATT9_, BATTA_, ..., BATTG_
QtObject {
    required property var controller
    required property int batteryIndex

    readonly property string _prefix: prefixForIndex(batteryIndex)

    /// Returns the parameter prefix for a 0-based battery index.
    /// 0: "BATT_", 1: "BATT2_", ..., 8: "BATT9_", 9: "BATTA_", ..., 15: "BATTG_"
    function prefixForIndex(index) {
        if (index === 0) {
            return "BATT_"
        }
        if (index <= 8) {
            return "BATT" + (index + 1) + "_"
        }
        return "BATT" + String.fromCharCode(65 + index - 9) + "_"
    }

    property Fact battMonitor: controller.getParameterFact(-1, _prefix + "MONITOR")
    property Fact battCapacity: controller.getParameterFact(-1, _prefix + "CAPACITY", false)
    property Fact battArmVolt: controller.getParameterFact(-1, _prefix + "ARM_VOLT", false)
    property Fact battAmpPerVolt: controller.getParameterFact(-1, _prefix + "AMP_PERVLT", false)
    property Fact battAmpOffset: controller.getParameterFact(-1, _prefix + "AMP_OFFSET", false)
    property Fact battCurrPin: controller.getParameterFact(-1, _prefix + "CURR_PIN", false)
    property Fact battVoltMult: controller.getParameterFact(-1, _prefix + "VOLT_MULT", false)
    property Fact battVoltPin: controller.getParameterFact(-1, _prefix + "VOLT_PIN", false)

    property bool monitorEnabled: battMonitor.rawValue !== 0
    property bool paramsAvailable: controller.parameterExists(-1, _prefix + "CAPACITY")
    property bool showReboot: monitorEnabled && !paramsAvailable
}
