import QtQuick
import QtMultimedia

import QGroundControl

// Displays a video stream: registers itself with the stream's controller, which hands it to the stream's receiver.
VideoOutput {
    id: root

    required property VideoStreamController stream

    property bool _completed: false
    // Qt defect, seen in Qt 6.12.0: QQuickVideoOutput::itemChange() never calls QQuickItem::itemChange()
    // (qtmultimedia/src/multimediaquick/qquickvideooutput.cpp), so a VideoOutput emits no windowChanged and its own
    // Window.window goes stale. Hence the parent's window; moving to another window (PiP pop-out) needs a
    // re-registration. Once the minimum Qt has the fix, QGCQVideoSinkController can connect to windowChanged, and
    // _window, _completed and on_WindowChanged can go.
    readonly property QtObject _window: parent ? parent.Window.window : null
    // A lost stream's last frame stays up while it reconnects; a stopped or replaced stream's is cleared.
    readonly property bool _keepsFrame: root.stream.decoding || root.stream.reconnecting

    // The rotation the camera reports for the stream (VIDEO_STREAM_INFORMATION.rotation). VideoOutput applies its
    // orientation on top of each frame's own rotation and mirroring, which qgcqvideosink takes from the stream's
    // orientation tag: a stream that carries a tag and also reports a rotation is turned by both.
    orientation: root.stream.rotation
    endOfStreamPolicy: VideoOutput.KeepLastFrame
    opacity: root.stream.decoding ? 1 : 0.5

    on_KeepsFrameChanged: {
        if (!root._keepsFrame) {
            root.clearOutput()
        }
    }

    on_WindowChanged: {
        if (root._completed && root._window) {
            root.stream.registerVideoOutput(root)
        }
    }

    Component.onCompleted: {
        root._completed = true
        root.stream.registerVideoOutput(root)
    }

    Component.onDestruction: root.stream.unregisterVideoOutput(root)
}
