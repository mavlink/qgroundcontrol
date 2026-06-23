import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    spacing: _margin
    visible: tabBar.currentIndex === 2

    property var missionItem

    MouseArea {
        Layout.preferredWidth:  childrenRect.width
        Layout.preferredHeight: childrenRect.height

        onClicked: {
            var removeModes = []
            var updateFunction = function(altFrame){ missionItem.cameraCalc.distanceMode = altFrame }
            removeModes.push(QGroundControl.AltitudeFrameMixed)
            if (!missionItem.masterController.controllerVehicle.supports.terrainFrame) {
                removeModes.push(QGroundControl.AltitudeFrameTerrain)
            }
            if (!QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || !missionItem.cameraCalc.isManualCamera) {
                removeModes.push(QGroundControl.AltitudeFrameAbsolute)
            }
            altFrameDialogFactory.open({ currentAltFrame: missionItem.cameraCalc.distanceMode, rgRemoveModes: removeModes, updateAltFrameFn: updateFunction })
        }

        QGCPopupDialogFactory {
            id: altFrameDialogFactory

            dialogComponent: altFrameDialogComponent
        }

        Component { id: altFrameDialogComponent; AltFrameDialog { } }

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth / 2

            QGCLabel { text: QGroundControl.altitudeFrameShortDescription(missionItem.cameraCalc.distanceMode) }
            QGCColoredImage {
                height:     ScreenTools.defaultFontPixelHeight / 2
                width:      height
                source:     "/res/DropArrow.svg"
                color:      qgcPal.text
            }
        }
    }

    GridLayout {
        Layout.fillWidth:   true
        columnSpacing:      _margin
        rowSpacing:         _margin
        columns:            2
        enabled:            missionItem.cameraCalc.distanceMode === QGroundControl.AltitudeFrameCalcAboveTerrain

        QGCLabel { text: qsTr("Tolerance") }
        FactTextField {
            fact:               missionItem.terrainAdjustTolerance
            Layout.fillWidth:   true
        }

        QGCLabel { text: qsTr("Max Climb Rate") }
        FactTextField {
            fact:               missionItem.terrainAdjustMaxClimbRate
            Layout.fillWidth:   true
        }

        QGCLabel { text: qsTr("Max Descent Rate") }
        FactTextField {
            fact:               missionItem.terrainAdjustMaxDescentRate
            Layout.fillWidth:   true
        }
    }
}
