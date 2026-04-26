pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.VideoManager

/// Top-level video display compositor.
/// Assembles VideoOutput, overlays, thermal view, and zoom handler
/// into a single layout-managed surface. No vehicle/camera resolution —
/// camera is an input property provided by the consumer.
Item {
    id:     root
    clip:   true

    property bool useSmallFont: true
    property var  camera:       null

    readonly property real videoWidth:  _sizeCalc.videoWidth
    readonly property real videoHeight: _sizeCalc.videoHeight

    // `activeStreamForRole()` is Q_INVOKABLE, not Q_PROPERTY — QML can't observe
    // the switch automatically. Mirror it into a plain property and refresh on
    // VideoStreamModel::activeStreamChanged(Primary) so downstream bindings
    // (decoding → _showVideo, videoSize → _sizeCalc) actually update.
    property var _primaryStream: null
    readonly property var _primarySize:   _primaryStream ? _primaryStream.videoSize : Qt.size(0, 0)

    property bool _showVideo: _primaryStream ? _primaryStream.decoding : false

    function _refreshPrimaryStream() {
        const m = QGroundControl.videoManager.streamModel
        _primaryStream = m ? m.activeStreamForRole(VideoStream.Primary) : null
    }

    Component.onCompleted: _refreshPrimaryStream()

    Connections {
        target: QGroundControl.videoManager.streamModel
        function onActiveStreamChanged(role) {
            if (role === VideoStream.Primary)
                _refreshPrimaryStream()
        }
    }

    VideoSizeCalculator {
        id:              _sizeCalc
        containerWidth:  root.width
        containerHeight: root.height
        aspectRatio:     root._primarySize.height > 0
                             ? root._primarySize.width / root._primarySize.height
                             : (root._primaryStream ? root._primaryStream.sourceAspectRatio : 0)
        fitMode:         QGroundControl.settingsManager.videoSettings.videoFit.rawValue
    }

    NoVideoPlaceholder {
        anchors.fill: parent
        visible:      !root._showVideo
        useSmallFont: root.useSmallFont
    }

    Rectangle {
        id:             videoBackground
        anchors.fill:   parent
        color:          "black"
        visible:        root._showVideo

        Item {
            id:                 videoContentArea
            height:             _sizeCalc.videoHeight
            width:              _sizeCalc.videoWidth
            anchors.centerIn:   parent
            // `visible` inherited from videoBackground parent — no duplicate binding needed.

            // Video surface is a direct child of videoContentArea (rather than
            // a sibling anchored to it) so reparenting or z-reordering cannot
            // silently break the layout binding. Kept alive (not in a Loader)
            // so the QVideoSink persists across reconnects and pipeline reuse
            // doesn't require sink re-registration on every hasVideo transition.
            QGCVideoOutput {
                anchors.fill: parent
                streamRole:   VideoStream.Primary
            }

            VideoStatsOverlay {
                anchors.top:    parent.top
                anchors.right:  parent.right
                anchors.margins: 4
                z: 10
            }

            VideoGridOverlay {}
        }

        ThermalVideoView {
            camera: root.camera
        }

        CameraZoomHandler {
            anchors.fill: parent
            camera:       root.camera
        }

        // Dynamic-stream thumbnail strip — one tile per Role::Dynamic stream,
        // laid out as a horizontal row along the bottom. Fixed layout per
        // design decision: compositor owns geometry, streams are naming-only.
        Row {
            id:                       dynamicStrip
            anchors.bottom:           parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin:     8
            spacing:                  4
            z:                        20

            readonly property real tileWidth:  200
            readonly property real tileHeight: 120

            Repeater {
                model: QGroundControl.videoManager.streamModel
                delegate: Loader {
                    id: tileLoader
                    // Model role names (see VideoStreamModel::roleNames).
                    required property var    stream
                    required property string streamName

                    // `stream.role` is a CONSTANT Q_PROPERTY on VideoStream.
                    active: tileLoader.stream && tileLoader.stream.role === VideoStream.Dynamic
                    sourceComponent: Rectangle {
                        width:        dynamicStrip.tileWidth
                        height:       dynamicStrip.tileHeight
                        color:        "black"
                        border.color: "white"
                        border.width: 1

                        QGCVideoOutput {
                            anchors.fill:    parent
                            anchors.margins: 1
                            streamName:      tileLoader.streamName
                            videoFillMode:   VideoOutput.PreserveAspectFit
                        }
                    }
                }
            }
        }
    }
}
