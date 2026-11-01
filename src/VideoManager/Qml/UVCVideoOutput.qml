import QtQuick
import QtMultimedia

import QGroundControl

// Displays the USB camera VideoManager selected, cropped to fill the item and rotated and mirrored as the settings say:
// registers itself with the camera's controller, which runs the camera while the output exists. The controller's
// screenshots follow this output's orientation and mirroring.
VideoOutput {
    id: root

    readonly property UVCCameraController _camera: QGroundControl.videoManager.uvcCamera
    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    fillMode: VideoOutput.PreserveAspectCrop
    orientation: root._videoSettings.uvcCameraRotation.rawValue
    mirrored: root._videoSettings.uvcCameraMirror.rawValue

    Component.onCompleted:      root._camera.registerVideoOutput(root)
    Component.onDestruction:    root._camera.unregisterVideoOutput(root)
}
