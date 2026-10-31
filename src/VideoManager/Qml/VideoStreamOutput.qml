import QtQuick
import QtMultimedia

import QGroundControl

// Displays a video stream: registers itself with the stream's controller, which hands it to the stream's receiver.
VideoOutput {
    id: _root

    // Do NOT set `orientation` here — VideoOutput composes orientation on top of the
    // QVideoFrame's own rotation()/mirrored() metadata that qgcqvideosink forwards from
    // GstVideoOrientationMeta. Setting it would double-rotate any stream with orientation tags.

    // videoFit enum: 0=Fit Width, 1=Fit Height, 2=Fill, 3=No Crop. The container
    // handles fit-width/fit-height sizing; only Fill needs the cropping fillMode.
    fillMode: QGroundControl.settingsManager.videoSettings.videoFit.rawValue === 2
              ? VideoOutput.PreserveAspectCrop
              : VideoOutput.PreserveAspectFit

    required property VideoStreamController stream

    property bool _completed: false
    // VideoOutput never emits windowChanged (QQuickVideoOutput::itemChange() doesn't chain up), so follow the
    // parent's window. Moving to another window (PiP pop-out) needs a re-registration.
    readonly property QtObject _window: parent ? parent.Window.window : null

    on_WindowChanged: {
        if (_completed && _window) {
            stream.registerVideoOutput(_root)
        }
    }

    Component.onCompleted: {
        _completed = true
        stream.registerVideoOutput(_root)
    }

    Component.onDestruction: stream.unregisterVideoOutput(_root)
}
