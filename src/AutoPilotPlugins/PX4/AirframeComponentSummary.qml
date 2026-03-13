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

    AirframeComponentController { id: controller; }

    property Fact sysIdFact:        controller.getParameterFact(-1, "MAV_SYS_ID")
    property Fact sysAutoStartFact: controller.getParameterFact(-1, "SYS_AUTOSTART")

    property bool autoStartSet: sysAutoStartFact ? (sysAutoStartFact.value !== 0) : false

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("System ID")
            valueText: sysIdFact ? sysIdFact.valueString : ""
        }
        VehicleSummaryRow {
            labelText: qsTr("Airframe type")
            valueText: autoStartSet ? controller.currentAirframeType : qsTr("Setup required")
        }
        VehicleSummaryRow {
            labelText: qsTr("Vehicle")
            valueText: autoStartSet ? controller.currentVehicleName : qsTr("Setup required")
        }

        VehicleSummaryRow {
            labelText: qsTr("Firmware Version")
            valueText: globals.activeVehicle.firmwareMajorVersion === -1 ? qsTr("Unknown") : globals.activeVehicle.firmwareMajorVersion + "." + globals.activeVehicle.firmwareMinorVersion + "." + globals.activeVehicle.firmwarePatchVersion + globals.activeVehicle.firmwareVersionTypeString
        }
        VehicleSummaryRow {
            visible: globals.activeVehicle.firmwareCustomMajorVersion !== -1
            labelText: qsTr("Custom Fw. Ver.")
            valueText: globals.activeVehicle.firmwareCustomMajorVersion + "." + globals.activeVehicle.firmwareCustomMinorVersion + "." + globals.activeVehicle.firmwareCustomPatchVersion
        }
    }
}
