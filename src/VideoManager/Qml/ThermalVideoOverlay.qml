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
    /// The manager whose thermal stream is shown.
    property VideoManager videoManager: QGroundControl.videoManager

    // Whether the thermal mode shows the overlay. Not the item's visibility, which also goes with the view it is in:
    // behind another view the stream keeps decoding, so that coming back doesn't wait for a keyframe.
    readonly property bool _shown: root.videoManager.hasThermal && !!root.camera
                                   && root.camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF
    // The ratio of the thermal picture as its output shows it, as for the main picture. Until its first frame, the one
    // the stream tells of, which keeps the overlay and what is placed beside it in shape.
    readonly property real _aspectRatio: (outputLoader.item && outputLoader.item.aspectRatio > 0)
                                         ? outputLoader.item.aspectRatio : root.videoManager.thermalStream.aspectRatio
    // Size of the thermal video blended over the main one, relative to the view.
    readonly property real _blendSizeFactor: 0.85
    // The view the overlay is placed in: none while it is created, and again while it is destroyed.
    readonly property real _viewWidth: root.parent ? root.parent.width : 0
    readonly property real _viewHeight: root.parent ? root.parent.height : 0
    // The box the thermal video fits in, as the thermal mode asks.
    readonly property size _box: {
        if (!root.camera) {
            return Qt.size(0, 0)
        }
        switch (root.camera.thermalMode) {
        case MavlinkCameraControlInterface.THERMAL_FULL:
            return Qt.size(root._viewWidth, root._viewHeight)
        case MavlinkCameraControlInterface.THERMAL_PIP:
            return Qt.size(Math.max(0, root._viewWidth - root.anchors.leftMargin),
                           ScreenTools.defaultFontPixelHeight * 12)
        default:
            return Qt.size(root._viewWidth * root._blendSizeFactor, root._viewHeight * root._blendSizeFactor)
        }
    }

    width:      Math.min(root._box.width, root._box.height * root._aspectRatio)
    height:     root.width / root._aspectRatio
    visible:    root._shown

    anchors {
        horizontalCenter:   root.parent ? root.parent.horizontalCenter : undefined
        verticalCenter:     root.parent ? root.parent.verticalCenter : undefined
        // Apply only in the pip state, which anchors the top left corner instead.
        topMargin:          root.pipTopInset + (ScreenTools.defaultFontPixelHeight * 0.5)
        leftMargin:         ScreenTools.defaultFontPixelWidth * 12
    }

    // The stream decodes while it has an output: none while the overlay isn't shown.
    Loader {
        id:                 outputLoader
        anchors.fill:       parent
        active:             root._shown
        opacity:            !root.camera ? 0
                            : (root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                               ? root.camera.thermalOpacity / 100 : 1.0)
        sourceComponent:    thermalOutputComponent

        Component {
            id: thermalOutputComponent

            VideoStreamOutput {
                stream: root.videoManager.thermalStream
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
