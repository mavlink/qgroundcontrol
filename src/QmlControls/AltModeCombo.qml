import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCComboBox {
    required property int altitudeMode
    required property var vehicle

    textRole: "modeName"

    onActivated: (index) => {
        let modeValue = altModeModel.get(index).modeValue
        altitudeMode = modeValue
    }

    ListModel {
        id: altModeModel

        ListElement {
            modeName: qsTr("Relative")
            modeValue: QGroundControl.AltitudeModeRelative
        }
        ListElement {
            modeName: qsTr("Absolute")
            modeValue: QGroundControl.AltitudeModeAbsolute
        }
        ListElement {
            modeName: qsTr("Terrain")
            modeValue: QGroundControl.AltitudeModeTerrainFrame
        }
        ListElement {
            modeName: qsTr("TerrainC")
            modeValue: QGroundControl.AltitudeModeCalcAboveTerrain
        }
    }

    Component.onCompleted: {
        let removeModes = []

        if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && altitudeMode != QGroundControl.AltitudeModeAbsolute) {
            removeModes.push(QGroundControl.AltitudeModeAbsolute)
        }
        if (!vehicle.supportsTerrainFrame) {
            removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
        }

        // Remove modes specified by consumer
        for (var i=0; i<removeModes.length; i++) {
            for (var j=0; j<altModeModel.count; j++) {
                if (altModeModel.get(j).modeValue == removeModes[i]) {
                    altModeModel.remove(j)
                    break
                }
            }
        }

        model = altModeModel

        // Find the specified alt mode in the model
        for (var k=0; k<altModeModel.count; k++) {
            if (altModeModel.get(k).modeValue == altitudeMode) {
                currentIndex = k
                break
            }
        }
    }
}
