import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

Item {
    id: root

    property var  parentToolInsets
    property real _heading:  NaN
    property real _pitch:    NaN
    property bool _inverted: false
    property var vehicle: globals.activeVehicle
    property real _rollAngle: (vehicle ? vehicle.roll.rawValue  : 0) + (_inverted ? 180 : 0)
    property int  cameraSlot
    property int index

    QGCPalette { id: qgcPalette }

    function wrapPitchFull(pitch) {
        var normalizedPitch = Math.abs(pitch % 360)          // 0..360
        var segment = Math.floor(normalizedPitch / 90)   // segment: 0,1,2,3
        var remainder = normalizedPitch - segment * 90           // remainder within segment: 0..90

        var newPitch, inverted
        switch (segment) {
        case 0: newPitch = remainder;         inverted = false; break  // 0   -> 90
        case 1: newPitch = 90 - remainder;    inverted = true;  break  // 90  -> 180 (back to 0, upside down)
        case 2: newPitch = -remainder;        inverted = true;  break  // 180 -> 270 (0 -> -90, still upside down)
        case 3: newPitch = -(90 - remainder); inverted = false; break  // 270 -> 360 (-90 -> 0, flips back normal)
        default: newPitch = 0;        inverted = false; break
        }

        return {
            pitch:    (pitch % 360) < 0 ? -newPitch : newPitch,
            inverted: inverted
        }
    }

    function resetCameraOverrides() {
        _heading = NaN
        _pitch = NaN
        _inverted = false
    }

    onCameraSlotChanged: resetCameraOverrides()

    Loader {
        anchors.right:  parent.right
        anchors.bottom: parent.bottom

        sourceComponent: {
            switch (SVSettings.compassType) {
            case "vertical":
                return verticalCompassComponent

            case "combined":
                return combinedCompassComponent

            default:
                return horizontalCompassComponent
            }
        }
    }

    Component {
        id: horizontalCompassComponent

        SVBackground {
            width:  attitude.width + compass.width + SVUnits.bigMargin * 3
            height: Math.max(attitude.height, compass.height) + SVUnits.bigMargin * 2

            enabled:        true
            borderColor:    qgcPalette.windowShade
            hoverEnabled:   false
            checkable:      false
            checked:        false
            hovered:        false
            pressed:        false
            borderWidth:    1
            radius:         height / 2

            QGCAttitudeWidget {
                id:                     attitude
                size:                   SVUnits.objectWidth * 1.4
                vehicle:                globals.activeVehicle
                pitchOverride:          root._pitch
                _rollAngle:             root._rollAngle
                anchors.left:           parent.left
                anchors.leftMargin:     SVUnits.bigMargin
                anchors.verticalCenter: parent.verticalCenter
            }

            QGCCompassWidget {
                id:                     compass
                anchors.left:           attitude.right
                anchors.leftMargin:     SVUnits.bigMargin
                size:                   SVUnits.objectWidth * 1.4
                vehicle:                globals.activeVehicle
                headingOverride:        root._heading
                border.width:             SVSettings.simplifiedUserInterface ? 0 : 1
                _lockNoseUpCompass:     true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Component {
        id: verticalCompassComponent

        SVBackground {
            width:  Math.max(attitude.width, compass.width) + SVUnits.bigMargin * 2
            height: attitude.height + compass.height + SVUnits.bigMargin * 3

            enabled:      true
            borderColor:  qgcPalette.windowShade
            hoverEnabled: false
            checkable:    false
            checked:      false
            hovered:      false
            pressed:      false
            borderWidth:  1
            radius:       height / 2

            QGCAttitudeWidget {
                id:                       attitude
                size:                     SVUnits.objectWidth * 1.4
                vehicle:                  globals.activeVehicle
                pitchOverride:            root._pitch
                _rollAngle:               root._rollAngle
                anchors.top:              parent.top
                anchors.topMargin:        SVUnits.bigMargin
                anchors.horizontalCenter: parent.horizontalCenter
            }

            QGCCompassWidget {
                id:                       compass
                anchors.top:              attitude.bottom
                anchors.topMargin:        SVUnits.bigMargin
                size:                     SVUnits.objectWidth * 1.4
                vehicle:                  globals.activeVehicle
                headingOverride:          root._heading
                border.width:             SVSettings.simplifiedUserInterface ? 0 : 1
                _lockNoseUpCompass:     true
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }

    Component {
        id: combinedCompassComponent

        Item {
            // Container bound matching the horizontal/vertical backgrounds
            width:  SVUnits.objectWidth * 1
            height: SVUnits.objectWidth * 1

            Rectangle {
                id: border
                anchors.left: combinedWidget.left
                anchors.bottom: combinedWidget.bottom
                anchors.leftMargin: -SVUnits.lineWidth
                anchors.bottomMargin: -SVUnits.lineWidth
                width: SVUnits.objectWidth * 2 + SVUnits.lineWidth * 2
                height: SVUnits.objectWidth * 2 + SVUnits.lineWidth * 2
                color: "white"
                radius: height / 2
                visible: !SVSettings.simplifiedUserInterface
            }

            IntegratedCompassAttitude {
                id: combinedWidget
                
                // Explicitly define attitude dimensions so child elements position properly
                attitudeSize:    SVUnits.objectWidth * 0.20
                attitudeSpacing: SVUnits.margin

                // Keep your max radius constraint
                maxCompassRadius: SVUnits.objectWidth * 1.0

                // Pass the overrides
                headingOverride: root._heading
                pitchOverride:   root._pitch
                rollOverride:    root._rollAngle

                // Center inside the container padding box, taking child negative offsets into account
                anchors.right: parent.right
                anchors.bottom: parent.bottom

                
            }
        }
    }

    SVCameraDetectionFlag {
        id: detectionFlag
        anchors.left: parent.left
        anchors.top: parent.top
        width: SVUnits.objectWidth
        height: SVUnits.objectWidth
        radius: SVUnits.radius
        index: root.cameraSlot

        // 1. Safely grab the state for this specific camera slot
        property var camState: (QGroundControl.digiviewManager && 
                                QGroundControl.digiviewManager.cameraStates && 
                                root.cameraSlot >= 0 && 
                                root.cameraSlot < QGroundControl.digiviewManager.cameraStates.length)
                               ? QGroundControl.digiviewManager.cameraStates[root.cameraSlot]
                               : null

        // 2. Make visible for backend or local tracking/selection state.
        visible: (camState ? camState.hasActiveTarget : false)
            || (root.cameraSlot >= 0 && root.cameraSlot < SVState.cameraTrackingIds.length
                && SVState.cameraTrackingIds[root.cameraSlot] !== "")
            || (SVState.cursorTrackingSessionActive
                && SVState.cursorTrackingSessionSlot === root.cameraSlot)

        // 3. Dynamically set the color based on the camera slot index
        // 3. Color reflects actual tracking state, not the slot index
        normalColor: qgcPalette.windowTransparent
        hoverColor: qgcPalette.windowShadeLight
        borderColor: qgcPalette.windowShadeLight
        borderHoverColor: "white"
    }

    Connections {
        target: QGroundControl.digiviewManager

        function onStreamNameChanged() {
            root.resetCameraOverrides()
        }

        function onCamTargetingParametersReceived(streamName, camId, _targetingMode, eulerDelta, yaw, pitch) {
            if (streamName !== QGroundControl.digiviewManager.streamName || camId !== root.cameraSlot) {
                return
            }

            if (eulerDelta !== 0 || !Number.isFinite(yaw) || !Number.isFinite(pitch)) {
                root.resetCameraOverrides()
                return
            }

            root._heading = (yaw % 360 + 360) % 360

            var result = root.wrapPitchFull(pitch)
            root._pitch    = result.pitch
            root._inverted = result.inverted
        }
    }
}
