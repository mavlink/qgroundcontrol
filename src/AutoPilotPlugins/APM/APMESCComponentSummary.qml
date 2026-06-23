import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

Item {
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: mainLayout.implicitHeight
    width: parent.width

    FactPanelController { id: controller; }

    property bool _isQuadPlane: !controller.parameterExists(-1, "MOT_PWM_TYPE") && controller.parameterExists(-1, "Q_M_PWM_TYPE")
    property string _escPrefix: _isQuadPlane ? "Q_M_" : "MOT_"

    property bool _motPwmTypeAvailable: controller.parameterExists(-1, _escPrefix + "PWM_TYPE")
    property Fact _motPwmType: controller.getParameterFact(-1, _escPrefix + "PWM_TYPE", false /* reportMissing */)

    property bool _isDshot: _motPwmTypeAvailable && _motPwmType && _motPwmType.rawValue >= 4
    property bool _servoDshotEscAvailable: controller.parameterExists(-1, "SERVO_DSHOT_ESC")
    property Fact _servoDshotEsc: controller.getParameterFact(-1, "SERVO_DSHOT_ESC", false /* reportMissing */)
    property bool _servoDshotRateAvailable: controller.parameterExists(-1, "SERVO_DSHOT_RATE")
    property Fact _servoDshotRate: controller.getParameterFact(-1, "SERVO_DSHOT_RATE", false /* reportMissing */)

    ColumnLayout {
        id: mainLayout
        spacing: 0

        VehicleSummaryRow {
            labelText: qsTr("Output type")
            valueText: _motPwmTypeAvailable ? _motPwmType.enumStringValue : ""
            visible: _motPwmTypeAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("DShot ESC type")
            valueText: _servoDshotEscAvailable ? _servoDshotEsc.enumStringValue : ""
            visible: _isDshot && _servoDshotEscAvailable
        }

        VehicleSummaryRow {
            labelText: qsTr("DShot output rate")
            valueText: _servoDshotRateAvailable ? _servoDshotRate.enumStringValue : ""
            visible: _isDshot && _servoDshotRateAvailable
        }
    }
}
