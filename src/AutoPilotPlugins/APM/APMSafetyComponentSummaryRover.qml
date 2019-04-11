/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    FactPanelController { id: controller; }

    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")
    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "FS_THR_VALUE")
    property Fact _failsafeAction:      controller.getParameterFact(-1, "FS_ACTION")
    property Fact _failsafeCrashCheck:  controller.getParameterFact(-1, "FS_CRASH_CHECK")

    property Fact _armingCheck:         controller.getParameterFact(-1, "ARMING_CHECK")

    property string _failsafeActionText
    property string _failsafeCrashCheckText

    Component.onCompleted: {
        setFailsafeActionText()
        setFailsafeCrashCheckText()
    }

    Connections {
        target: _failsafeAction

        onValueChanged: setFailsafeActionText()
    }

    Connections {
        target: _failsafeCrashCheck

        onValueChanged: setFailsafeCrashCheckText()
    }

    function setFailsafeActionText() {
        switch (_failsafeAction.value) {
        case 0:
            _failsafeActionText = qsTr("Disabled")
            break
        case 1:
            _failsafeActionText = qsTr("Always RTL")
            break
        case 2:
            _failsafeActionText = qsTr("Always Hold")
            break
        default:
            _failsafeActionText = qsTr("Unknown")
        }
    }

    function setFailsafeCrashCheckText() {
        switch (_failsafeCrashCheck.value) {
        case 0:
            _failsafeCrashCheckText = qsTr("Disabled")
            break
        case 1:
            _failsafeCrashCheckText = qsTr("Hold")
            break
        case 2:
            _failsafeCrashCheckText = qsTr("Hold and Disarm")
            break
        default:
            _failsafeCrashCheckText = qsTr("Unknown")
        }
    }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText:  _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Throttle failsafe:")
            valueText:  _failsafeThrEnable.value != 0 ? _failsafeThrValue.valueString : qsTr("Disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Failsafe Action:")
            valueText: _failsafeActionText
        }

        VehicleSummaryRow {
            labelText: qsTr("Failsafe Crash Check:")
            valueText: _failsafeCrashCheckText
        }

    }
}
