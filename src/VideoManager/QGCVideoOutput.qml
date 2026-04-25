import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.VideoManager

// Unified video output for QtMultimedia display receivers.
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

    property var _targetStream: _resolveStream()

    onStreamNameChanged: _targetStream = _resolveStream()
    onStreamRoleChanged: _targetStream = _resolveStream()

    function _resolveStream() {
        const m = QGroundControl.videoManager.streamModel
        if (!m) return null
        if (root.streamName.length > 0) return m.stream(root.streamName)
        if (root.streamRole >= 0) return m.activeStreamForRole(root.streamRole)
        return null
    }

    VideoSinkBinder {
        stream: root._targetStream
        videoSink: root.videoSink
    }

    Connections {
        target: QGroundControl.videoManager.streamModel
        function onActiveStreamChanged(role) {
            if (root.streamName.length > 0 || role === root.streamRole) {
                root._targetStream = root._resolveStream()
            }
        }
    }
}
