import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// How a network stream is received: Low Latency Mode or, without it, the RTP jitter latency of GStreamer or of the
// experimental FFmpeg backend. Shown only for a network stream.
QGCGroupBox {
    id: root

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    heading: qsTr("Video Stream")
    visible: QGroundControl.videoManager.isStreamSource

    FactCheckBoxSlider {
        objectName:         "videoLowLatencyMode"
        text:               qsTr("Low Latency Mode")
        fact:               root._videoSettings.lowLatencyMode
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }

    LabelledFactTextField {
        objectName: "videoRtpJitterLatency"
        label:      qsTr("RTP jitter latency")
        fact:       root._videoSettings.rtpJitterLatencyMs
        // As VideoDecodingSettingsGroup's _decoderSettings: GStreamer's alone in a build without the FFmpeg backend.
        visible:    fact.userVisible && !root._videoSettings.lowLatencyMode.rawValue
                    && (root._videoSettings.hasFFmpegBackend
                        ? root._videoSettings.videoBackend.rawValue !== VideoSettings.VideoBackendQtMultimedia
                        : root._videoSettings.videoBackend.rawValue === VideoSettings.VideoBackendGStreamer)
    }
}
