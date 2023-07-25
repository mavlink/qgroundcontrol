import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Layouts          1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ArduPilot     1.0

/*
    IMPORTANT NOTE: Any changes made here must also be made to SensorsComponentSummary.qml
*/

Item {
    anchors.fill:   parent

    APMSensorsComponentController { id: controller; }

    APMSensorParams {
        id:                     sensorParams
        factPanelController:    controller
    }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
        labelText:  qsTr("Compasses:")
        valueText: ""
        }

        Repeater {
            model: sensorParams.rgCompassAvailable.length
            RowLayout {
                Layout.fillWidth: true
                width: parent.width

                QGCLabel {

                    text:  sensorParams.rgCompassAvailable[index] ?
                                (sensorParams.rgCompassCalibrated[index] ?
                                     getPriority(index) +
                                     (sensorParams.rgCompassExternalParamAvailable[index] ?
                                          (sensorParams.rgCompassExternal[index] ? ", External" : ", Internal" ) :
                                          "") :
                                     qsTr("Setup required")) :
                                qsTr("Not installed")

                    function getPriority (index) {
                        if (sensorParams.rgCompassId[index].value == sensorParams.rgCompassPrio[0].value) {
                            return "Primary"
                        }
                        if (sensorParams.rgCompassId[index].value == sensorParams.rgCompassPrio[1].value) {
                            return "Secondary"
                        }
                        if (sensorParams.rgCompassId[index].value == sensorParams.rgCompassPrio[2].value) {
                            return "Tertiary"
                        }
                        return "Unused"
                    }
                }

                APMSensorIdDecoder {
                    horizontalAlignment:    Text.AlignRight
                    Layout.alignment:       Qt.AlignRight

                    fact: sensorParams.rgCompassPrio[index]
                }
            }
        }

        VehicleSummaryRow {
            labelText: qsTr("Accelerometer(s):")
            valueText: controller.accelSetupNeeded ? qsTr("Setup required") : qsTr("Ready")
        }

        Repeater {
            model: sensorParams.rgInsId.length
            APMSensorIdDecoder {
                fact:          sensorParams.rgInsId[index]
                anchors.right: parent.right
            }
        }

        VehicleSummaryRow {
            labelText: qsTr("Barometer(s):")
            valueText: sensorParams.baroIdAvailable ? "" : qsTr("Not Supported(Over APM 4.1)")
        }

        Repeater {
            model: sensorParams.rgBaroId.length
            APMSensorIdDecoder {
                fact:          sensorParams.rgBaroId[index]
                anchors.right: parent.right
            }
        }
    }
}
