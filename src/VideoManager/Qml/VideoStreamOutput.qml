import QtQuick
import QtMultimedia

import QGroundControl

// Displays a video stream: registers itself with the stream's controller, which hands it to the stream's receiver.
VideoOutput {
    id: root

    /// The stream shown. The output follows a change of it: the main picture's stream is another one once a stream
    /// in a tile takes its place.
    required property VideoStreamController stream

    property bool _completed: false
    property VideoStreamController _registeredStream: null
    // Qt defect, seen in Qt 6.12.0: QQuickVideoOutput::itemChange() never calls QQuickItem::itemChange()
    // (qtmultimedia/src/multimediaquick/qquickvideooutput.cpp), so a VideoOutput emits no windowChanged and its own
    // Window.window goes stale. Hence the parent's window; moving to another window (PiP pop-out) needs a
    // re-registration. Once the minimum Qt has the fix, QGCQVideoSinkController can connect to windowChanged, and
    // _window, _completed and on_WindowChanged can go.
    readonly property QtObject _window: parent ? parent.Window.window : null
    // A lost stream's last frame stays up while it reconnects; a stopped or replaced stream's is cleared.
    readonly property bool _keepsFrame: root.stream.decoding || root.stream.reconnecting

    // Registers with the stream, anew if it is the same one. The stream it leaves keeps playing into the output that
    // takes over from this one, if one registers in the same pass.
    function _register() {
        if (root._registeredStream && root._registeredStream !== root.stream) {
            root._registeredStream.unregisterVideoOutput(root)
        }
        root._registeredStream = root.stream
        root.stream.registerVideoOutput(root)
    }

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
            root._register()
        }
    }

    onStreamChanged: {
        if (root._completed) {
            root._register()
        }
    }

    Component.onCompleted: {
        root._completed = true
        root._register()
    }

    Component.onDestruction: {
        if (root._registeredStream) {
            root._registeredStream.unregisterVideoOutput(root)
        }
    }
}
