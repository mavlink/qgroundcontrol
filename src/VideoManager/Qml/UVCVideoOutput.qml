import QtQuick
import QtMultimedia

import QGroundControl

// The UVC camera VideoManager selected, cropped to fill the item. VideoManager.uvcMonitor measures its frames.
VideoOutput {
    id: root

    readonly property VideoManager _videoManager: QGroundControl.videoManager

    fillMode: VideoOutput.PreserveAspectCrop

    Component.onCompleted:      root._videoManager.uvcMonitor.attach(uvcCamera, root.videoSink)
    Component.onDestruction:    root._videoManager.uvcMonitor.detach(root.videoSink)

    MediaDevices {
        id: mediaDevices

        function findCameraDevice(cameraId) {
            const videoInputs = mediaDevices.videoInputs
            for (let i = 0; i < videoInputs.length; i++) {
                if (videoInputs[i].description === cameraId) {
                    return videoInputs[i]
                }
            }
            return mediaDevices.defaultVideoInput
        }
    }

    Camera {
        id:             uvcCamera
        cameraDevice:   mediaDevices.findCameraDevice(root._videoManager.uvcVideoSourceID)
        active:         root._videoManager.isUvc
    }

    CaptureSession {
        camera:         uvcCamera
        videoOutput:    root
    }
}
