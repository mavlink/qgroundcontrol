import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    id: root

    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    readonly property bool _arspdTypeAvailable: controller.parameterExists(-1, "ARSPD_TYPE")
    readonly property Fact _arspdType: _arspdTypeAvailable ? controller.getParameterFact(-1, "ARSPD_TYPE") : null
    readonly property bool _sensorEnabled: _arspdTypeAvailable && _arspdType.rawValue !== 0
    readonly property bool _arspdUseAvailable: controller.parameterExists(-1, "ARSPD_USE")
    readonly property Fact _arspdUse: _arspdUseAvailable ? controller.getParameterFact(-1, "ARSPD_USE") : null
    readonly property bool _arspd2TypeAvailable: controller.parameterExists(-1, "ARSPD2_TYPE")
    readonly property Fact _arspd2Type: _arspd2TypeAvailable ? controller.getParameterFact(-1, "ARSPD2_TYPE") : null
    readonly property bool _cruiseAvailable: controller.parameterExists(-1, "AIRSPEED_CRUISE")
    readonly property Fact _cruise: _cruiseAvailable ? controller.getParameterFact(-1, "AIRSPEED_CRUISE") : null

    FactPanelController { id: controller }

    ColumnLayout {
        id: mainLayout

        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Sensor type")
            valueText: _arspdTypeAvailable ? _arspdType.enumStringValue : qsTr("N/A")
        }

        VehicleSummaryRow {
            labelText: qsTr("Use airspeed")
            valueText: _arspdUseAvailable ? _arspdUse.enumStringValue : qsTr("N/A")
            visible: _sensorEnabled
        }

        VehicleSummaryRow {
            labelText: qsTr("Sensor 2 type")
            valueText: _arspd2TypeAvailable ? _arspd2Type.enumStringValue : qsTr("N/A")
            visible: _arspd2TypeAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("Cruise airspeed")
            valueText: _cruiseAvailable ? _cruise.valueString + " " + _cruise.units : qsTr("N/A")
            visible: _sensorEnabled && _cruiseAvailable
        }
    }
}
