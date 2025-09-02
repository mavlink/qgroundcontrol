import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

ToolIndicatorPage {
    showExpand: true   // allow expansion (like GPS)

    property var    _activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property string na:      qsTr("N/A", "No data to display")
    property string valueNA: qsTr("--.--", "No data to display")

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            SettingsGroupLayout {
                heading: qsTr(" Flow Sensor Status")

                LabelledLabel {
                    label:      qsTr("Flow Rate")
                    labelText:  _activeVehicle&& !isNaN(_activeVehicle.flowSensor.flowRate.value)
                                ? _activeVehicle.flowSensor.flowRate.value.toFixed(2)
                                : "--"
                }

                LabelledLabel {
                    label:      qsTr("Pulse Count")
                    labelText:  _activeVehicle&& !isNaN(_activeVehicle.flowSensor.pulseCount.value)
                                ? _activeVehicle.flowSensor.pulseCount.value.toFixed(0)
                                : "--"          // placeholder
                }


            }
        }
    }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading: qsTr("Flow Settings")

            QGCLabel {
                text: qsTr("Additional flow sensor settings go here")
            }
        }
    }
}
