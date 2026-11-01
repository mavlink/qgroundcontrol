import QtQuick
import QtMultimedia

import QGroundControl

// Displays a video stream: registers itself with the stream's controller, which hands it to the stream's receiver.
VideoOutput {
    id: root

    required property VideoStreamController stream

    property bool _completed: false
    // VideoOutput never emits windowChanged (QQuickVideoOutput::itemChange() doesn't chain up), so follow the
    // parent's window. Moving to another window (PiP pop-out) needs a re-registration.
    readonly property QtObject _window: parent ? parent.Window.window : null
    // A lost stream's last frame stays up while it reconnects; a stopped or replaced stream's is cleared.
    readonly property bool _keepsFrame: root.stream.decoding || root.stream.reconnecting

    // Do NOT set `orientation` here — VideoOutput composes orientation on top of the
    // QVideoFrame's own rotation()/mirrored() metadata that qgcqvideosink forwards from
    // GstVideoOrientationMeta. Setting it would double-rotate any stream with orientation tags.

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
