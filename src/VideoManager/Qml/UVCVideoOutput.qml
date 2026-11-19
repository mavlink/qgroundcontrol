import QtQuick
import QtMultimedia

import QGroundControl

// Displays a USB camera: registers itself with the camera's controller, which runs the camera while the output exists,
// and turns and mirrors the picture as the camera's rotation and mirror settings say.
VideoOutput {
    id: root

    /// The camera shown: the one VideoManager selected.
    property UVCCameraController camera: QGroundControl.videoManager.uvcCamera

    // On top of each frame's own rotation and mirroring, as a phone's camera reports its sensor's.
    orientation:    root.camera.rotation
    mirrored:       root.camera.mirrored

    Component.onCompleted:      root.camera.registerVideoOutput(root)
    Component.onDestruction:    root.camera.unregisterVideoOutput(root)
}
