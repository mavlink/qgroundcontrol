import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

/// Altitude, altitude frame and firmware specific help of a rally point
ColumnLayout {
    id: root

    required property var rallyPoint
    required property var vehicle

    readonly property bool  _px4:   vehicle ? vehicle.px4Firmware : false
    readonly property bool  _apm:   vehicle ? vehicle.apmFirmware : false
    readonly property var   _frames: {
        const frames = [
            { text: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameRelative), value: QGroundControl.AltitudeFrameRelative },
            { text: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameAbsolute), value: QGroundControl.AltitudeFrameAbsolute }
        ]
        if (rallyPoint.altitudeFrame === QGroundControl.AltitudeFrameTerrain || (vehicle && vehicle.supports.terrainFrame)) {
            frames.push({ text: QGroundControl.altitudeFrameExtraUnits(QGroundControl.AltitudeFrameTerrain), value: QGroundControl.AltitudeFrameTerrain })
        }
        return frames
    }
    readonly property int   _frameIndex: _frames.findIndex(frame => frame.value === rallyPoint.altitudeFrame)
    readonly property string _helpText: {
        if (_px4) {
            return qsTr("Ground altitude at this point. Return and descent altitudes are measured from it.")
        } else if (_apm) {
            return qsTr("Flight altitude at this point. Plane flies to this altitude and loiters there. Copter returns at the higher of this altitude and RTL_ALT, then lands. Set it high enough to clear terrain and obstacles.")
        }
        return qsTr("Altitude of this point. How it is used depends on the vehicle firmware.")
    }

    spacing: ScreenTools.defaultFontPixelHeight * 0.25

    QGCPalette { id: qgcPal }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth

        LabelledFactTextField {
            objectName:         "rallyPointAltitudeField"
            Layout.fillWidth:   true
            label:              qsTr("Altitude")
            fact:               root.rallyPoint.altitude
        }

        QGCComboBox {
            objectName:     "rallyPointAltitudeFrameCombo"
            model:          root._frames
            textRole:       "text"
            currentIndex:   root._frameIndex

            onActivated: (index) => {
                // ArduPilot altitudes are flight altitudes, so a frame change starts from the default mission altitude
                const seedAltitude = root._apm ? QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude.rawValue : 0
                // PX4 AMSL is the ground elevation, ArduPilot AMSL is a flight altitude that terrain data does not give
                root.rallyPoint.changeAltitudeFrame(root._frames[index].value, seedAltitude, root._px4)
                // Activation breaks the binding. For PX4 AMSL the frame changes once the terrain query answers.
                currentIndex = Qt.binding(() => root._frameIndex)
            }
        }
    }

    QGCLabel {
        objectName:         "rallyPointAltitudeHelp"
        Layout.fillWidth:   true
        wrapMode:           Text.WordWrap
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.textSecondary
        text:               root._helpText
    }

    QGCLabel {
        objectName:         "rallyPointTerrainFailed"
        Layout.fillWidth:   true
        wrapMode:           Text.WordWrap
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.warningText
        text:               root.rallyPoint.altitudeFrame === QGroundControl.AltitudeFrameAbsolute
                                ? qsTr("Terrain elevation is unavailable. Enter the AMSL altitude manually.")
                                : qsTr("Terrain elevation is unavailable. Select AMSL to enter the altitude manually.")
        visible:            root.rallyPoint.terrainQueryFailed
    }
}
