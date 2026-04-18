import QtQuick
import QtMultimedia

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

// Thermal video overlay — supports FULL, PIP, and BLEND modes.
// Layout driven by QML State + AnchorChanges (no imperative anchor mutation).
Item {
    id: root

    required property var camera
    property double thermalHeightFactor: 0.85

    /// Top margin applied in PIP mode, provided by the hosting window.
    /// Default of 0 lets the view work in isolation (e.g. unit tests or
    /// previews) without relying on an ambient `mainWindow` id.
    property real pipTopMarginOffset: 0

    // `activeStreamForRole()` is Q_INVOKABLE — QML won't re-evaluate a direct
    // binding on activeStreamChanged. Mirror the lookup into a plain property
    // and refresh it via the Connections block below.
    property var _thermalStream: null
    readonly property var _thermalInfo: _thermalStream ? _thermalStream.videoStreamInfo : null

    function _refreshThermalStream() {
        const m = QGroundControl.videoManager.streamModel
        _thermalStream = m ? m.activeStreamForRole(VideoStream.Thermal) : null
    }

    Component.onCompleted: _refreshThermalStream()

    Connections {
        target: QGroundControl.videoManager.streamModel
        function onActiveStreamChanged(role) {
            if (role === VideoStream.Thermal)
                _refreshThermalStream()
        }
    }

    width:  height * (_thermalInfo ? _thermalInfo.aspectRatio : 1.0)
    // Default height for BLEND mode; overridden in states for FULL and PIP.
    height: camera ? parent.height * thermalHeightFactor : 0
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter:   parent.verticalCenter
    visible: (_thermalInfo ? _thermalInfo.isThermal : false)
             && camera
             && camera.thermalMode !== MavlinkCameraControlInterface.THERMAL_OFF

    states: [
        State {
            name: "full"
            when: root.camera && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_FULL
            PropertyChanges {
                root.height: root.camera ? root.parent.height : 0
            }
        },
        State {
            name: "pip"
            when: root.camera && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_PIP
            PropertyChanges {
                root.height: ScreenTools.defaultFontPixelHeight * 12
            }
            AnchorChanges {
                target: root
                anchors.horizontalCenter: undefined
                anchors.verticalCenter:   undefined
                anchors.top:              root.parent.top
                anchors.left:             root.parent.left
            }
            PropertyChanges {
                root.anchors.topMargin:  root.pipTopMarginOffset + (ScreenTools.defaultFontPixelHeight * 0.5)
                root.anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 12
            }
        }
    ]

    // Kept alive (not in a Loader) so the QVideoSink persists across reconnects
    // and pipeline reuse doesn't require sink re-registration on every visibility
    // transition. Opacity drives the thermal blend effect instead.
    QGCVideoOutput {
        anchors.fill:  parent
        streamRole:    VideoStream.Thermal
        opacity: root.camera
                     ? (root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
                            ? root.camera.thermalOpacity / 100 : 1.0)
                     : 0
        // ThermalVideoView sizes itself to the thermal aspect ratio
        // directly (not via VideoSizeCalculator), so let the VideoOutput
        // element preserve aspect inside the parent Loader bounds.
        videoFillMode: VideoOutput.PreserveAspectFit
    }
}
