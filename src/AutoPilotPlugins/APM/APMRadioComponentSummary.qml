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

    property Fact mapRollFact:      controller.getParameterFact(-1, "RCMAP_ROLL")
    property Fact mapPitchFact:     controller.getParameterFact(-1, "RCMAP_PITCH")
    property Fact mapYawFact:       controller.getParameterFact(-1, "RCMAP_YAW")
    property Fact mapThrottleFact:  controller.getParameterFact(-1, "RCMAP_THROTTLE")

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Roll")
            valueText: mapRollFact.value == 0 ? qsTr("Setup required") : qsTr("Channel %1").arg(mapRollFact.valueString)
        }

        VehicleSummaryRow {
            labelText: qsTr("Pitch")
            valueText: mapPitchFact.value == 0 ? qsTr("Setup required") : qsTr("Channel %1").arg(mapPitchFact.valueString)
        }

        VehicleSummaryRow {
            labelText: qsTr("Yaw")
            valueText: mapYawFact.value == 0 ? qsTr("Setup required") : qsTr("Channel %1").arg(mapYawFact.valueString)
        }

        VehicleSummaryRow {
            labelText: qsTr("Throttle")
            valueText: mapThrottleFact.value == 0 ? qsTr("Setup required") : qsTr("Channel %1").arg(mapThrottleFact.valueString)
        }
    }
}
