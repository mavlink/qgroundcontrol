pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

// The thermal stream over the main video, placed as the camera's thermal mode asks: fitting the view, blended over the
// video, or as a picture-in-picture in the top left corner.
Item {
    id: root

    /// The camera whose thermal mode and opacity the overlay follows; any object with those properties will do.
    property var camera: null
    /// Height covered at the top of the view, which the picture-in-picture stays below.
    property real pipTopInset: 0

    readonly property VideoManager _videoManager: QGroundControl.videoManager
    readonly property real _aspectRatio: root._videoManager.thermalAspectRatio
    // Size of the thermal video blended over the main one, relative to the view.
    readonly property real _blendSizeFactor: 0.85
    // The view the overlay is placed in: none while it is created, and again while it is destroyed.
    readonly property real _viewWidth: root.parent ? root.parent.width : 0
    readonly property real _viewHeight: root.parent ? root.parent.height : 0
    // The box the thermal video fits in, as the thermal mode asks.
    readonly property real _boxWidth: {
        if (!root.camera) {
            return 0
        }
        switch (root.camera.thermalMode) {
        case MavlinkCameraControlInterface.THERMAL_FULL:
            return root._viewWidth
        case MavlinkCameraControlInterface.THERMAL_PIP:
            return Math.max(0, root._viewWidth - root.anchors.leftMargin)
        default:
            return root._viewWidth * root._blendSizeFactor
        }
    }
    readonly property real _boxHeight: {
        if (!root.camera) {
            return 0
        }
        switch (root.camera.thermalMode) {
        case MavlinkCameraControlInterface.THERMAL_FULL:
            return root._viewHeight
        case MavlinkCameraControlInterface.THERMAL_PIP:
            return ScreenTools.defaultFontPixelHeight * 12
        default:
            return root._viewHeight * root._blendSizeFactor
        }
    }

    width:      Math.min(root._boxWidth, root._boxHeight * root._aspectRatio)
    height:     root.width / root._aspectRatio
    visible:    root._videoManager.hasThermal && !!root.camera
                && root.camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF

    anchors {
        horizontalCenter:   root.parent ? root.parent.horizontalCenter : undefined
        verticalCenter:     root.parent ? root.parent.verticalCenter : undefined
        // Apply only in the pip state, which anchors the top left corner instead.
        topMargin:          root.pipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5)
        leftMargin:         ScreenTools.defaultFontPixelWidth * 12
    }

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

    // In a group of its own: an item's states are a list, which a user of this type would add to, not replace.
    StateGroup {
        states: [
            State {
                name:   "pip"
                when:   !!root.parent && !!root.camera
                        && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP

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
    }
}
