import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width  // grows when Loader is wider than implicitWidth

    FactPanelController { id: controller; }

    property Fact _copterFenceAction: controller.getParameterFact(-1, "FENCE_ACTION", false /* reportMissing */)
    property Fact _copterFenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE", false /* reportMissing */)
    property Fact _copterFenceType:   controller.getParameterFact(-1, "FENCE_TYPE", false /* reportMissing */)

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText: {
                if (_armingCheckFact) {
                    return _armingCheckFact.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
                } else if (_armingSkipCheckFact) {
                    return _armingSkipCheckFact.value === 0 ? qsTr("Enabled") : qsTr("Some disabled")
                }
                return ""
            }

            // Older firmwares use ARMING_CHECK. Newer firmwares use ARMING_SKIPCHK.
            property Fact _armingCheckFact:     controller.getParameterFact(-1, "ARMING_CHECK", false /* reportMissing */)
            property Fact _armingSkipCheckFact: controller.getParameterFact(-1, "ARMING_SKIPCHK", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: {
                if(_copterFenceEnable && _copterFenceType) {
                    if(_copterFenceEnable.value == 0 || _copterFenceType.value == 0) {
                        return qsTr("Disabled")
                    } else {
                        if(_copterFenceType.value == 1) {
                            return qsTr("Altitude")
                        }
                        if(_copterFenceType.value == 2) {
                            return qsTr("Circle")
                        }
                        return qsTr("Altitude,Circle")
                    }
                }
                return ""
            }
            visible: controller.vehicle.multiRotor
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: _copterFenceAction ? (_copterFenceAction.value == 0 ?
                           qsTr("Report only") :
                           (_copterFenceAction.value == 1 ? qsTr("RTL or Land") : qsTr("Unknown"))) : ""
            visible: controller.vehicle.multiRotor && _copterFenceEnable && _copterFenceEnable.value !== 0
        }

        VehicleSummaryRow {
            labelText:  qsTr("RTL min alt:")
            valueText:  fact ? (fact.value == 0 ? qsTr("current") : fact.valueString + " " + fact.units) : ""
            visible:    controller.vehicle.multiRotor

            property Fact fact: controller.getParameterFact(-1, "RTL_ALT_M", false /* reportMissing */)
        }

        VehicleSummaryRow {
            labelText:  qsTr("RTL min alt:")
            valueText:  fact ? (fact.value < 0 ? qsTr("current") : fact.valueString + " " + fact.units) : ""
            visible:    controller.vehicle.fixedWing

            property Fact fact: controller.getParameterFact(-1, "RTL_ALTITUDE", false /* reportMissing */)
        }
    }
}
