import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VideoManager

/// Debug overlay showing live video statistics (FPS, resolution, drops, latency, health).
/// Toggle via VideoSettings.showVideoStats.
Item {
    id: root

    property bool showStats: QGroundControl.settingsManager.videoSettings.showVideoStats.rawValue

    readonly property real _fontPointSize: ScreenTools.smallFontPointSize

    // Resolve the primary stream. `streamForRole()` is Q_INVOKABLE (not a
    // Q_PROPERTY) so its result isn't reactive — the binding must depend on a
    // signal that fires when the active stream pointer changes. `_tick` is
    // incremented by the Connections block below when `activeStreamChanged`
    // fires for our role, forcing the binding to re-evaluate. Without this,
    // the QML overlay resolves `_primary` once at instantiation (before
    // VideoManager::_createVideoStreams runs) and binds to null forever.
    property int _tick: 0
    readonly property var _primary: {
        root._tick; // binding dependency — do not remove
        return QGroundControl.videoManager.streamModel.activeStreamForRole(VideoStream.Primary)
    }

    Connections {
        target: QGroundControl.videoManager.streamModel
        function onActiveStreamChanged(role) {
            if (role === VideoStream.Primary)
                root._tick++
        }
    }

    visible: showStats
    width:  statsLoader.item ? statsLoader.item.width  + 16 : 0
    height: statsLoader.item ? statsLoader.item.height + 12 : 0

    Rectangle {
        anchors.fill: parent
        color: "#80000000"
        radius: 4
    }

    // Wrap the Text tree in a Loader so the nodes are only instantiated
    // when the overlay is actually visible (showStats = true). This avoids
    // unnecessary QML binding evaluations for all stat properties while
    // the overlay is hidden — especially relevant when streaming is inactive.
    Loader {
        id: statsLoader
        active: root.showStats
        anchors.centerIn: parent
        sourceComponent: Component {
            ColumnLayout {
                spacing: 2

                Text {
                    text: "FPS: " + (root._primary ? root._primary.fps.toFixed(1) : "0.0")
                    color: "white"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
                Text {
                    readonly property size sz: root._primary ? root._primary.videoSize : Qt.size(0, 0)
                    text: "Res: " + sz.width + "x" + sz.height
                    color: "white"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
                Text {
                    property real latMs: root._primary ? root._primary.latencyMs : -1
                    visible: root._primary ? root._primary.latencySupported : true
                    text: "Lat: " + (latMs >= 0 ? latMs.toFixed(1) + " ms" : "—")
                    color: latMs > 100 ? "#ff6666" : latMs > 50 ? "#ffcc00" : "white"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
                Text {
                    readonly property int dropped: root._primary ? root._primary.droppedFrames : 0
                    text: "Drop: " + dropped
                    color: dropped > 0 ? "#ffcc00" : "white"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
                Text {
                    property int health: root._primary ? root._primary.streamHealth : VideoStreamStats.Good
                    text: "Health: " + (health === VideoStreamStats.Good ? "Good"
                                      : health === VideoStreamStats.Degraded ? "Degraded"
                                      : "Critical")
                    color: health === VideoStreamStats.Good ? "#66ff66"
                         : health === VideoStreamStats.Degraded ? "#ffcc00"
                         : "#ff6666"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
                Text {
                    readonly property string decName: root._primary ? root._primary.activeDecoderName : ""
                    readonly property bool   hw:      root._primary ? root._primary.hwDecoding        : false
                    visible: decName.length > 0
                    text: "Dec: " + decName + (hw ? " (HW)" : " (SW)")
                    color: hw ? "#66ff66" : "white"
                    font.pointSize: root._fontPointSize
                    font.family: "monospace"
                }
            }
        }
    }
}
