import QtQuick
import QtMultimedia

import QGroundControl

// Displays the USB camera VideoManager selected, cropped to fill the item: registers itself with the camera's
// controller, which runs the camera while the output exists.
VideoOutput {
    id: root

    readonly property UVCCameraController _camera: QGroundControl.videoManager.uvcCamera

    fillMode: VideoOutput.PreserveAspectCrop

    Component.onCompleted:      root._camera.registerVideoOutput(root)
    Component.onDestruction:    root._camera.unregisterVideoOutput(root)
}
