import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABLE")
    property Fact _failsafeLeakEnable:  controller.getParameterFact(-1, "FS_LEAK_ENABLE")

    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

    property Fact _leakPin:     controller.getParameterFact(-1, "WD_1_PIN")

    property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

    property string _failsafeGCSEnableText

    Component.onCompleted: {
        setFailsafeGCSEnableText()
    }

    Connections {
        target: _failsafeGCSEnable

        onValueChanged: setFailsafeGCSEnableText()
    }

    function setFailsafeGCSEnableText() {
        switch (_failsafeGCSEnable.value) {
        case 0:
            _failsafeGCSEnableText = qsTr("Disabled")
            break
        case 1:
            _failsafeGCSEnableText = qsTr("Always RTL")
            break
        case 2:
            _failsafeGCSEnableText = qsTr("Continue with Mission in Auto Mode")
            break
        default:
            _failsafeGCSEnableText = qsTr("Unknown")
        }
    }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText: qsTr("Arming Checks:")
            valueText:  _armingCheck.value & 1 ? qsTr("Enabled") : qsTr("Some disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("GCS failsafe:")
            valueText: _failsafeGCSEnableText
        }

        VehicleSummaryRow {
            labelText: qsTr("Leak failsafe:")
            valueText:  _failsafeLeakEnable.value ? qsTr("Enabled") : qsTr("Disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("Leak Detector:")
            valueText: _leakPin.value > 0 ? qsTr("Enabled") : qsTr("Disabled")
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: _fenceEnable.value == 0 || _fenceType == 0 ?
                           qsTr("Disabled") :
                           (_fenceType.value == 1 ?
                                qsTr("Depth") :
                                (_fenceType.value == 2 ? qsTr("Circle") : qsTr("Depth,Circle")))
        }

        VehicleSummaryRow {
            labelText: qsTr("GeoFence:")
            valueText: qsTr("Report only")
            visible:    _fenceEnable.value != 0
        }
    }
}
