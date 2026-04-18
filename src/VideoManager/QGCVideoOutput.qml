import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.VideoManager

// Unified video output for all receiver types (GStreamer, QtMultimedia, UVC).
// Registers its QVideoSink with the VideoStream identified by `streamName`
// (preferred) or `streamRole` (for Primary/Thermal/UVC singleton slots).
// Deregisters on destruction and on active-stream swaps.
VideoOutput {
    id: root

    /// Role of the stream this output is connected to (VideoStream.Primary,
    /// VideoStream.Thermal, etc.). Used when streamName is empty.
    property int streamRole: -1

    /// Stable stream identity — takes precedence over streamRole when set.
    /// Used by Dynamic streams where multiple streams share Role::Dynamic.
    property string streamName: ""

    /// Default fill mode is Stretch because VideoCompositor pre-sizes its
    /// container via VideoSizeCalculator to the correct aspect ratio; applying
    /// PreserveAspectFit inside would double-fit and can leave a sub-pixel
    /// black border. Consumers that embed this element directly (no
    /// VideoSizeCalculator around them — e.g. ThermalVideoView) override
    /// this to PreserveAspectFit.
    property int videoFillMode: VideoOutput.Stretch

    fillMode: videoFillMode
    endOfStreamPolicy: VideoOutput.KeepLastFrame

    // Track the stream we registered our sink with so role swaps deregister
    // the old stream before registering the new one. Without this, a stopped
    // stream's bridge keeps a live sink pointer and can race with the new
    // stream's bridge on setVideoFrame, or silently hold a dangling reference
    // past its natural lifetime.
    property var _registeredStream: null

    function _resolveStream() {
        const m = QGroundControl.videoManager.streamModel
        if (!m) return null
        if (root.streamName.length > 0) return m.stream(root.streamName)
        if (root.streamRole >= 0) return m.activeStreamForRole(root.streamRole)
        return null
    }

    function _rebindSink() {
        const next = _resolveStream()
        if (next === _registeredStream) return
        if (_registeredStream) _registeredStream.registerVideoSink(null)
        _registeredStream = next
        if (_registeredStream) _registeredStream.registerVideoSink(root.videoSink)
    }

    Component.onCompleted: _rebindSink()

    Component.onDestruction: {
        if (_registeredStream) {
            _registeredStream.registerVideoSink(null)
            _registeredStream = null
        }
    }

    Connections {
        target: QGroundControl.videoManager.streamModel
        function onActiveStreamChanged(role) {
            // Name-bound outputs don't care about active-stream swaps — their
            // target stream is fixed for its lifetime.
            if (root.streamName.length > 0) return
            if (role === root.streamRole) _rebindSink()
        }
    }
}
