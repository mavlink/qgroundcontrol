import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools


ToolIndicatorPage {
    showExpand: true

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 4

            SettingsGroupLayout {
                heading: qsTr("Distance Sensor Debug - All Orientations")

                Repeater {
                    model: [
                        { key: _activeVehicle.distanceSensors.rotationNone,     label: "Forward" },
                        // { key: _activeVehicle.distanceSensors.rotationYaw45,    label: "Forward/Right (45°)" },
                        // { key: _activeVehicle.distanceSensors.rotationYaw90,     label: "Right (90°)" },
                        // { key: _activeVehicle.distanceSensors.rotationYaw135,   label: "Rear/Right (135°)" },
                        { key: _activeVehicle.distanceSensors.rotationYaw180,   label: "Rear" },
                        // { key: "rotationYaw225",   label: "Rear/Left (225°)" },
                        // { key: "rotationYaw270",   label: "Left (270°)" },
                        // { key: "rotationYaw315",   label: "Forward/Left (315°)" },
                        // { key: "rotationPitch90",  label: "Up (Pitch 90°)" },
                        { key: _activeVehicle.distanceSensors.rotationPitch270, label: "Down" }
                    ]
                     delegate: LabelledLabel {
                        label: modelData.label
                        labelText: _activeVehicle && _activeVehicle.distanceSensors && !isNaN(modelData.key.value) ?
                                       modelData.key.value.toFixed(2) + " m"
                                   : "--"
                    }
                }
            }
        }
    }
// }

    expandedComponent: Component {
        SettingsGroupLayout {
            heading: qsTr("Distance Sensor Settings")

            QGCLabel {
                text: qsTr("Additional Distance sensor settings go here")
            }
        }
    }
}
