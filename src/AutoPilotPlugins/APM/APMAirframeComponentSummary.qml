import QtQuick 2.2
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.Palette 1.0

FactPanel {
    id:             panel
    anchors.fill:   parent
    color:          qgcPal.windowShadeDark

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    APMAirframeComponentController {
        id:         controller
        factPanel:  panel
    }

    property bool _useOldFrameParam:    controller.parameterExists(-1, "FRAME")
    property Fact _oldFrameParam:       controller.getParameterFact(-1, "FRAME", false)
    property Fact _newFrameParam:       controller.getParameterFact(-1, "FRAME_CLASS", false)
    property Fact _frameTypeParam:      controller.getParameterFact(-1, "FRAME_TYPE", false)

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText:  qsTr("Frame Type:")
            valueText:  controller.currentAirframeTypeName() + " " + _oldFrameParam.enumStringValue
            visible:    _useOldFrameParam
        }

        VehicleSummaryRow {
            labelText:  qsTr("Frame Class:")
            valueText:  _newFrameParam.enumStringValue
            visible:    !_useOldFrameParam

        }

        VehicleSummaryRow {
            labelText:  qsTr("Frame Type:")
            valueText:  _frameTypeParam.enumStringValue
            visible:    !_useOldFrameParam

        }

        VehicleSummaryRow {
            labelText: qsTr("Firmware Version:")
            valueText: activeVehicle.firmwareMajorVersion == -1 ? qsTr("Unknown") : activeVehicle.firmwareMajorVersion + "." + activeVehicle.firmwareMinorVersion + "." + activeVehicle.firmwarePatchVersion + activeVehicle.firmwareVersionTypeString
        }
    }
}
