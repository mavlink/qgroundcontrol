import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// How a network stream is received: Low Latency Mode or, without it, GStreamer's RTP jitter buffer latency. Shown only
// for a network stream.
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
        visible:    fact.userVisible && !root._videoSettings.lowLatencyMode.rawValue
                    && root._videoSettings.videoBackend.rawValue === VideoSettings.VideoBackendGStreamer
    }
}
