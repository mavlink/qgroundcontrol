import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    Column {
        anchors.fill: parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText: {
                if (_armingCheck) {
                    return _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
                } else if (_armingSkipCheck) {
                    return _armingSkipCheck.value === 0 ? qsTr("Enabled") : qsTr("Some disabled")
                }
                return ""
            }

            // Older firmwares use ARMING_CHECK. Newer firmwares use ARMING_SKIPCHK.
            property Fact _armingCheck:     controller.getParameterFact(-1, "ARMING_CHECK", false /* reportMissing */)
            property Fact _armingSkipCheck: controller.getParameterFact(-1, "ARMING_SKIPCHK", false /* reportMissing */)
        }
    }
}
