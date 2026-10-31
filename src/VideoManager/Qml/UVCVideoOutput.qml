import QtQuick
import QtMultimedia

import QGroundControl

Rectangle {
    id:                 _root
    width:              parent.width
    height:             parent.height
    implicitWidth:      videoOutput.implicitWidth
    implicitHeight:     videoOutput.implicitHeight
    color:              Qt.rgba(0,0,0,0.75)
    clip:               true
    anchors.centerIn:   parent
    visible:            _videoManager.isUvc

    property var _videoManager: QGroundControl.videoManager

    MediaDevices {
        id: mediaDevices

        function findCameraDevice(cameraId) {
            var videoInputs = mediaDevices.videoInputs
            for (var i = 0; i < videoInputs.length; i++) {
                if (videoInputs[i].description === cameraId) {
                    return videoInputs[i]
                }
            }
            return mediaDevices.defaultVideoInput
        }
    }

    CaptureSession {
        camera: Camera {
            id:             camera
            cameraDevice:   mediaDevices.findCameraDevice(_videoManager.uvcVideoSourceID)
            active:         _videoManager.isUvc
        }
        videoOutput: videoOutput
    }

    VideoOutput {
        id:             videoOutput
        anchors.fill:   parent
        fillMode:       VideoOutput.PreserveAspectCrop
    }
}
