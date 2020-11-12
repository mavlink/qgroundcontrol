/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

// Exposes the set of battery parameters for new and old firmwares
//  Older firmware: BAT_* naming
//  Newer firmware: BAT#_* naming, with indices starting at 1
QtObject {
    property var controller     ///< FactPanelController
    property int batteryIndex   ///< 1-based battery index


    property Fact battSource:                   controller.getParameterFact(-1, "BAT#_SOURCE".replace       ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property Fact battNumCells:                 controller.getParameterFact(-1, "BAT#_N_CELLS".replace      ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property Fact battHighVolt:                 controller.getParameterFact(-1, "BAT#_V_CHARGED".replace    ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property Fact battLowVolt:                  controller.getParameterFact(-1, "BAT#_V_EMPTY".replace      ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property Fact battVoltLoadDrop:             controller.getParameterFact(-1, "BAT#_V_LOAD_DROP".replace  ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property Fact battVoltageDivider:           controller.getParameterFact(-1, "BAT#_V_DIV".replace        ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""), false)
    property Fact battAmpsPerVolt:              controller.getParameterFact(-1, "BAT#_A_PER_V".replace      ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""), false)

    property bool battVoltageDividerAvailable:  controller.parameterExists(-1, "BAT#_V_DIV".replace     ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))
    property bool battAmpsPerVoltAvailable:     controller.parameterExists(-1, "BAT#_A_PER_V".replace   ("#", _indexedBatteryParamsAvailable ? batteryIndex : ""))

    property string _batNCellsIndexedParamName:     "BAT#_N_CELLS"
    property bool   _indexedBatteryParamsAvailable: controller.parameterExists(-1, _batNCellsIndexedParamName.replace("#", 1))
    property int    _indexedBatteryParamCount:      getIndexedBatteryParamCount()

    Component.onCompleted: {
        if (batteryIndex > 1 && !_indexedBatteryParamsAvailable) {
            console.warn("Internal Error: BatteryParams.qml batteryIndex > 1 while indexed params are not available", batteryIndex)
        }
    }

    function getIndexedBatteryParamCount() {
        var batteryIndex = 1
        do {
            if (!controller.parameterExists(-1, _batNCellsIndexedParamName.replace("#", batteryIndex))) {
                return batteryIndex - 1
            }
            batteryIndex++
        } while (true)
    }
}
