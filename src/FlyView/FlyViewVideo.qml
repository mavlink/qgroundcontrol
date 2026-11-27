import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

Item {
    id: root

    property Item pipView
    /// Height the Fly View toolbar covers at the top of the full-size video.
    property real topInset: 0
    /// Width the Fly View's widgets cover at the top right of the full-size video.
    property real rightInset: 0
    property Item pipState: videoPipState
    /// The manager whose video the view shows.
    property VideoManager videoManager: QGroundControl.videoManager

    PipState {
        id:         videoPipState
        pipView:    root.pipView
        isDark:     true

        onStateChanged: {
            if (videoPipState.state !== videoPipState.fullState) {
                root.videoManager.fullScreen = false
            }
        }
    }

    //-- Video Streaming
    VideoView {
        id:                 videoStreaming
        anchors.fill:       parent
        useSmallFont:       root.pipState.state !== root.pipState.fullState
        thermalPipTopInset: root.pipState.state === root.pipState.fullState ? root.topInset : 0
        showTiles:          root.pipState.state === root.pipState.fullState
        tilesRightInset:    root.rightInset
        videoManager:       root.videoManager
        showVideo:          root.videoManager.isStreamSource || root.videoManager.isUvc
        interactive:        root.pipState.state === root.pipState.fullState

        onClicked: (x, y) => {
            onScreenGimbalController.mouseClicked(x, y)
            cameraTrackingController.mouseClicked(x, y)
        }
        onDragStarted: (x, y) => {
            onScreenGimbalController.mouseDragStart(x, y)
            cameraTrackingController.mouseDragStart(x, y)
        }
        onDragMoved: (x, y) => {
            onScreenGimbalController.mouseDragPositionChanged(x, y)
            cameraTrackingController.mouseDragPositionChanged(x, y)
        }
        onDragEnded: (x, y) => {
            onScreenGimbalController.mouseDragEnd()
            cameraTrackingController.mouseDragEnd(x, y)
        }
    }

    OnScreenGimbalController {
        id:                      onScreenGimbalController
        anchors.fill:            parent
        cameraTrackingEnabled:   !!(videoStreaming.camera && videoStreaming.camera.trackingEnabled)
    }

    OnScreenCameraTrackingController {
        id:                      cameraTrackingController
        anchors.fill:            parent
        camera:                  videoStreaming.camera
        videoWidth:              videoStreaming.videoWidth
        videoHeight:             videoStreaming.videoHeight
    }

    ProximityRadarVideoView{
        anchors.fill:   parent
        vehicle:        QGroundControl.multiVehicleManager.activeVehicle
    }

    ObstacleDistanceOverlayVideo {
        id: obstacleDistance
        showText: root.pipState.state === root.pipState.fullState
    }
}
