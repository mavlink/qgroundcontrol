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

    // Do NOT set `orientation` here — VideoOutput composes orientation on top of the
    // QVideoFrame's own rotation()/mirrored() metadata that qgcqvideosink forwards from
    // GstVideoOrientationMeta. Setting it would double-rotate any stream with orientation tags.

    // The container sizes the video for the other fit modes; only Fill needs the cropping fillMode.
    fillMode: QGroundControl.settingsManager.videoSettings.videoFit.rawValue === VideoSettings.VideoFitFill
              ? VideoOutput.PreserveAspectCrop
              : VideoOutput.PreserveAspectFit

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
