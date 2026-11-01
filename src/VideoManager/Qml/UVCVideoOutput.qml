import QtQuick
import QtMultimedia

import QGroundControl

// The UVC camera VideoManager selected, cropped to fill the item.
Rectangle {
    id: root

    readonly property VideoManager _videoManager: QGroundControl.videoManager

    implicitWidth:  uvcOutput.implicitWidth
    implicitHeight: uvcOutput.implicitHeight
    color:          Qt.rgba(0, 0, 0, 0.75)

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
        videoOutput: uvcOutput
    }

    VideoOutput {
        id:             uvcOutput
        anchors.fill:   parent
        fillMode:       VideoOutput.PreserveAspectCrop
    }
}
