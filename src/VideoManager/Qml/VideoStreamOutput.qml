import QtQuick
import QtMultimedia

import QGroundControl

// Displays a video source, a stream or the USB camera: registers itself with the source's controller, which hands it to
// the source's receiver. The picture has the video's own ratio, or the one forced for the stream, to which it is
// stretched.
VideoOutput {
    id: root

    /// The source shown. The output follows a change of it: the main picture's source is another one once a source
    /// in a tile takes its place.
    required property VideoStreamController stream
    /// Crops the picture to the output, instead of fitting it inside. In place of fillMode, which the output sets.
    property bool crop: false

    /// Width over height the picture is stretched to, and a screenshot of it with it; 0 for the video's own.
    readonly property real forcedAspectRatio: root.stream.forcedAspectRatio
    /// Width over height of the picture as shown: the forced ratio, else the video's own, which the first frame
    /// tells; 0 until then.
    readonly property real aspectRatio: root.forcedAspectRatio > 0
                                        ? root.forcedAspectRatio
                                        : (root.implicitHeight > 0 ? root.implicitWidth / root.implicitHeight : 0)

    // The output's shape over the forced one: above 1 where the output is the wider of the two.
    readonly property real _shapeRatio: (root.forcedAspectRatio > 0 && root.width > 0 && root.height > 0)
                                        ? root.width / (root.height * root.forcedAspectRatio) : 1
    // The forced picture takes the output's width, and its height follows; else the other way round.
    readonly property bool _widthBound: (root._shapeRatio > 1) === root.crop
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

    // The rotation the camera reports for the stream (VIDEO_STREAM_INFORMATION.rotation), or the rotation and the
    // mirroring set for the USB camera. VideoOutput applies them on top of each frame's own rotation and mirroring,
    // which qgcqvideosink takes from the stream's orientation tag: a stream that carries a tag and also reports a
    // rotation is turned by both.
    orientation: root.stream.rotation
    mirrored: root.stream.mirrored
    endOfStreamPolicy: VideoOutput.KeepLastFrame
    opacity: root.stream.decoding ? 1 : 0.5
    // A forced ratio fills the output with the video, then scales it to the picture's size about the centre.
    fillMode: root.forcedAspectRatio > 0 ? VideoOutput.Stretch
                                         : (root.crop ? VideoOutput.PreserveAspectCrop : VideoOutput.PreserveAspectFit)
    transform: Scale {
        origin.x: root.width / 2
        origin.y: root.height / 2
        xScale: root._widthBound ? 1 : 1 / root._shapeRatio
        yScale: root._widthBound ? root._shapeRatio : 1
    }

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
