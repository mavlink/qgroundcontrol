import QtQuick
import QtMultimedia

import QGroundControl

// The UVC camera VideoManager selected, cropped to fill the item.
VideoOutput {
    id: root

    readonly property VideoManager _videoManager: QGroundControl.videoManager

    fillMode: VideoOutput.PreserveAspectCrop

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

    CaptureSession {
        camera: Camera {
            cameraDevice:   mediaDevices.findCameraDevice(root._videoManager.uvcVideoSourceID)
            active:         root._videoManager.isUvc
        }
        videoOutput: root
    }
}
