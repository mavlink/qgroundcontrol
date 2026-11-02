import QtQuick
import QtMultimedia

import QGroundControl

// Displays the USB camera VideoManager selected: registers itself with the camera's controller, which runs the camera
// while the output exists. The controller hands it the camera's frames turned and
// mirrored as the settings say, so the output keeps its own orientation, and the subtitle Qt draws over it (the
// telemetry overlay) stays upright.
VideoOutput {
    id: root

    readonly property UVCCameraController _camera: QGroundControl.videoManager.uvcCamera

    Component.onCompleted:      root._camera.registerVideoOutput(root)
    Component.onDestruction:    root._camera.unregisterVideoOutput(root)
}
