import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// The health of one video stream, from its controller's stats; shown only while the stream is measured. The camera's
// advertised values follow the measured ones when they are known.
QGCGroupBox {
    id: root

    required property VideoStreamController stream

    readonly property var    _stats:        stream.stats
    readonly property size   _videoSize:    stream.videoSize
    readonly property var    _info:         stream.streamInfo
    readonly property real   _cameraFps:    _info ? _info.framerate : 0
    readonly property real   _cameraKbps:   _info ? _info.bitrate / 1000 : 0
    readonly property size   _cameraSize:   _info ? _info.resolution : Qt.size(0, 0)
    readonly property bool   _cameraSizeDiffers: _cameraSize.width > 0 && (_cameraSize.width !== _videoSize.width
                                                                          || _cameraSize.height !== _videoSize.height)
    readonly property string _videoText:    _videoSize.width > 0
                                            ? qsTr("%1 × %2 %3").arg(_videoSize.width).arg(_videoSize.height)
                                                                 .arg(_stats.codec)
                                            : _stats.codec
    readonly property string _decoderText:  _stats.hardwareDecoder ? qsTr("%1 (hardware)").arg(_stats.decoder)
                                                                   : qsTr("%1 (software)").arg(_stats.decoder)

    function _bitrateText(kbps) {
        return kbps >= 1000 ? qsTr("%1 Mbit/s").arg((kbps / 1000).toFixed(2)) : qsTr("%1 kbit/s").arg(kbps.toFixed(0))
    }

    function _durationText(ms) {
        const totalSeconds = Math.floor(ms / 1000)
        const hours = Math.floor(totalSeconds / 3600)
        const minutes = Math.floor(totalSeconds / 60) % 60
        const seconds = String(totalSeconds % 60).padStart(2, "0")
        return hours > 0 ? qsTr("%1:%2:%3").arg(hours).arg(String(minutes).padStart(2, "0")).arg(seconds)
                         : qsTr("%1:%2").arg(minutes).arg(seconds)
    }

    function _sizeText(bytes) {
        const megabytes = bytes / (1024 * 1024)
        return megabytes >= 1024 ? qsTr("%1 GB").arg((megabytes / 1024).toFixed(2))
                                 : qsTr("%1 MB").arg(megabytes.toFixed(1))
    }

    function _withCamera(measured, camera) {
        return camera !== "" ? qsTr("%1 (camera %2)").arg(measured).arg(camera) : measured
    }

    visible: _stats.valid

    LabelledLabel {
        label:      qsTr("Source")
        labelText:  root.stream.transport
        visible:    root.stream.transport !== ""
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth * 2
        visible: root.stream.uri !== ""

        QGCLabel {
            Layout.fillWidth:   true
            text:               qsTr("URL")
        }

        QGCLabel {
            Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 40
            text:                   root.stream.uri
            elide:                  Text.ElideMiddle
        }
    }

    LabelledLabel {
        label:      qsTr("Camera stream")
        labelText:  root._info ? root._info.name : ""
        visible:    labelText !== ""
    }

    LabelledLabel {
        label:      qsTr("Frame rate")
        labelText:  root._withCamera(qsTr("%1 fps").arg(root._stats.fps.toFixed(1)),
                                     root._cameraFps > 0 ? qsTr("%1 fps").arg(root._cameraFps.toFixed(0)) : "")
    }

    LabelledLabel {
        label:      qsTr("Dropped")
        labelText:  qsTr("%n frame(s)", "", root._stats.framesDropped)
    }

    LabelledLabel {
        label:      qsTr("Bitrate")
        labelText:  root._withCamera(root._bitrateText(root._stats.bitrateKbps),
                                     root._cameraKbps > 0 ? root._bitrateText(root._cameraKbps) : "")
    }

    LabelledLabel {
        label:      qsTr("Video")
        labelText:  root._withCamera(root._videoText, root._cameraSizeDiffers
                                     ? qsTr("%1 × %2").arg(root._cameraSize.width).arg(root._cameraSize.height) : "")
        visible:    root._videoText !== ""
    }

    LabelledLabel {
        label:      qsTr("Keyframe interval")
        labelText:  qsTr("%1 s").arg((root._stats.keyframeIntervalMs / 1000).toFixed(1))
        visible:    root._stats.keyframeIntervalMs >= 0
    }

    LabelledLabel {
        label:      qsTr("Decoder")
        labelText:  root._decoderText
        visible:    root._stats.decoder !== ""
    }

    LabelledLabel {
        label:      qsTr("Decoder restarts")
        labelText:  root._stats.decoderRestarts
        visible:    root._stats.decoderRestarts > 0
    }

    LabelledLabel {
        label:      qsTr("Display path")
        labelText:  root._stats.gpuPath
        visible:    root._stats.gpuPath !== ""
    }

    LabelledLabel {
        label:      qsTr("GPU fallbacks")
        labelText:  qsTr("%n frame(s)", "", root._stats.gpuFallbacks)
        visible:    root._stats.gpuFallbacks > 0
    }

    LabelledLabel {
        label:      qsTr("Latency")
        labelText:  qsTr("%1 ms").arg(root._stats.latencyMs.toFixed(0))
        visible:    root._stats.latencyMs >= 0
    }

    LabelledLabel {
        label:      qsTr("Packet loss")
        labelText:  qsTr("%1% (%n packet(s))", "", root._stats.packetsLost).arg(root._stats.lossPercent.toFixed(1))
        visible:    root._stats.hasRtpStats
    }

    LabelledLabel {
        label:      qsTr("Jitter")
        labelText:  qsTr("%1 ms").arg(root._stats.jitterMs.toFixed(1))
        visible:    root._stats.hasRtpStats
    }

    LabelledLabel {
        label:      qsTr("Late / duplicate packets")
        labelText:  qsTr("%1 / %2").arg(root._stats.packetsLate).arg(root._stats.packetsDuplicate)
        visible:    root._stats.hasRtpStats
    }

    LabelledLabel {
        label:      qsTr("Retransmissions")
        labelText:  qsTr("%1 requested, %2 recovered").arg(root._stats.rtxRequested).arg(root._stats.rtxRecovered)
        visible:    root._stats.rtxRequested > 0
    }

    LabelledLabel {
        label:      qsTr("Uptime")
        labelText:  root._durationText(root._stats.uptimeMs)
    }

    LabelledLabel {
        label:      qsTr("Reconnects")
        labelText:  root._stats.reconnects
        visible:    root._stats.reconnects > 0
    }

    LabelledLabel {
        label:      qsTr("Recording")
        labelText:  qsTr("%1, %2").arg(root._durationText(root._stats.recordingMs))
                                  .arg(root._sizeText(root._stats.recordingBytes))
        visible:    root._stats.recordingMs >= 0
    }
}
