import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCComboBox {
    required property int altitudeFrame
    required property var vehicle

    textRole: "modeName"

    onActivated: (index) => {
        let modeValue = altFrameModel.get(index).modeValue
        altitudeFrame = modeValue
    }

    ListModel {
        id: altFrameModel
    }

    Component.onCompleted: {
        altFrameModel.append({ modeName: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameRelative),
                              modeValue: QGroundControl.AltitudeFrameRelative })
        altFrameModel.append({ modeName: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameAbsolute),
                              modeValue: QGroundControl.AltitudeFrameAbsolute })
        altFrameModel.append({ modeName: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameTerrain),
                              modeValue: QGroundControl.AltitudeFrameTerrain })
        altFrameModel.append({ modeName: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameCalcAboveTerrain),
                              modeValue: QGroundControl.AltitudeFrameCalcAboveTerrain })

        let removeModes = []

        if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude && altitudeFrame != QGroundControl.AltitudeFrameAbsolute) {
            removeModes.push(QGroundControl.AltitudeFrameAbsolute)
        }
        if (!vehicle.supports.terrainFrame) {
            removeModes.push(QGroundControl.AltitudeFrameTerrain)
        }

        // Remove modes specified by consumer
        for (var i=0; i<removeModes.length; i++) {
            for (var j=0; j<altFrameModel.count; j++) {
                if (altFrameModel.get(j).modeValue == removeModes[i]) {
                    altFrameModel.remove(j)
                    break
                }
            }
        }

        model = altFrameModel

        // Find the specified alt mode in the model
        for (var k=0; k<altFrameModel.count; k++) {
            if (altFrameModel.get(k).modeValue == altitudeFrame) {
                currentIndex = k
                break
            }
        }
    }
}
