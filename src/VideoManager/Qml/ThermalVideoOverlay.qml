pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// The thermal stream over the main video, placed as the camera's thermal mode asks: filling the view's height, blended
// over the video, or as a picture-in-picture in the top left corner.
Item {
    id: root

    /// The camera whose thermal mode and opacity the overlay follows; any object with those properties will do.
    property var camera: null
    /// Height covered at the top of the view, which the picture-in-picture stays below.
    property real pipTopInset: 0

    readonly property VideoManager _videoManager: QGroundControl.videoManager
    // Height of the thermal video blended over the main one, relative to the view.
    readonly property real _blendHeightFactor: 0.85

    width:      height * root._videoManager.thermalAspectRatio
    height: {
        if (!root.camera) {
            return 0
        }
        switch (root.camera.thermalMode) {
        case MavlinkCameraControlInterface.THERMAL_FULL:
            return parent.height
        case MavlinkCameraControlInterface.THERMAL_PIP:
            return ScreenTools.defaultFontPixelHeight * 12
        default:
            return parent.height * root._blendHeightFactor
        }
    }
    visible:    root._videoManager.hasThermal && !!root.camera
                && root.camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF

    anchors {
        horizontalCenter:   parent.horizontalCenter
        verticalCenter:     parent.verticalCenter
        // Apply only in the pip state, which anchors the top left corner instead.
        topMargin:          root.pipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5)
        leftMargin:         ScreenTools.defaultFontPixelWidth * 12
    }

    states: [
        State {
            name:   "pip"
            when:   !!root.camera && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP

            AnchorChanges {
                target: root
                anchors {
                    horizontalCenter:   undefined
                    verticalCenter:     undefined
                    top:                root.parent.top
                    left:               root.parent.left
                }
            }
        }
    ]

    Loader {
        anchors.fill:       parent
        opacity:            !root.camera ? 0
                            : (root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                               ? root.camera.thermalOpacity / 100 : 1.0)
        sourceComponent:    thermalOutputComponent

        Component {
            id: thermalOutputComponent

            VideoStreamOutput {
                stream: root._videoManager.thermalStream
            }
        }
    }
}
