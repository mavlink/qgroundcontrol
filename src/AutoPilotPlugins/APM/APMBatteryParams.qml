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

    /// Returns a display label for a 0-based battery index.
    /// 0: "1", 1: "2", ..., 8: "9", 9: "A", ..., 15: "G"
    function labelForIndex(index) {
        if (index <= 8) {
            return String(index + 1)
        }
        return String.fromCharCode(65 + index - 9)
    }

    /// Returns the number of battery slots whose MONITOR parameter exists.
    function getBatteryCount() {
        for (let i = 0; i < 16; i++) {
            if (!controller.parameterExists(-1, prefixForIndex(i) + "MONITOR")) {
                return i
            }
        }
        return 16
    }

    /// Returns how many batteries have MONITOR enabled (rawValue != 0).
    function getEnabledBatteryCount() {
        let count = 0
        let total = getBatteryCount()
        for (let i = 0; i < total; i++) {
            let mon = controller.getParameterFact(-1, prefixForIndex(i) + "MONITOR", false)
            if (mon && mon.rawValue !== 0) count++
        }
        return count
    }

    property Fact battMonitor: controller.getParameterFact(-1, _prefix + "MONITOR")
    property Fact battCapacity: controller.getParameterFact(-1, _prefix + "CAPACITY", false)
    property Fact battArmVolt: controller.getParameterFact(-1, _prefix + "ARM_VOLT", false)
    property Fact battAmpPerVolt: controller.getParameterFact(-1, _prefix + "AMP_PERVLT", false)
    property Fact battAmpOffset: controller.getParameterFact(-1, _prefix + "AMP_OFFSET", false)
    property Fact battVoltMult: controller.getParameterFact(-1, _prefix + "VOLT_MULT", false)

    // Failsafe parameters
    property Fact fsLowAct: controller.getParameterFact(-1, _prefix + "FS_LOW_ACT", false)
    property Fact fsCritAct: controller.getParameterFact(-1, _prefix + "FS_CRT_ACT", false)
    property Fact lowMah: controller.getParameterFact(-1, _prefix + "LOW_MAH", false)
    property Fact critMah: controller.getParameterFact(-1, _prefix + "CRT_MAH", false)
    property Fact lowVolt: controller.getParameterFact(-1, _prefix + "LOW_VOLT", false)
    property Fact critVolt: controller.getParameterFact(-1, _prefix + "CRT_VOLT", false)

    property bool monitorEnabled: battMonitor.rawValue !== 0
    property bool paramsAvailable: controller.parameterExists(-1, _prefix + "CAPACITY")
    property bool showReboot: monitorEnabled && !paramsAvailable

    // ArduPilot BattMonitor::Type enum values for analog sensor detection
    readonly property int _monitorAnalogVoltageOnly:                3
    readonly property int _monitorAnalogVoltageAndCurrent:          4
    readonly property int _monitorAnalogVoltageSyntheticCurrent:    25
    readonly property int _monitorAnalogCurrentOnly:                31

    function _isMonitorTypeAnyOf(types) {
        return monitorEnabled && types.includes(battMonitor.rawValue)
    }

    readonly property bool hasVoltageSensor: _isMonitorTypeAnyOf([_monitorAnalogVoltageOnly, _monitorAnalogVoltageAndCurrent, _monitorAnalogVoltageSyntheticCurrent])
    readonly property bool hasCurrentSensor: _isMonitorTypeAnyOf([_monitorAnalogVoltageAndCurrent, _monitorAnalogCurrentOnly])
}
