import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Which backend and decoder the video streams use and whether their frames reach the display through the CPU; the
// decoder and CPU path are GStreamer's. A change restarts the streams. Shown only for a network stream, whether it is
// the main picture or in a tile beside a USB camera: they don't apply to the camera.
QGCGroupBox {
    id: root

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings
    readonly property bool _gstreamer: root._videoSettings.videoBackend.rawValue === VideoSettings.VideoBackendGStreamer

    heading:    qsTr("Decoding")
    visible:    QGroundControl.videoManager.isStreamSource
                && (root._videoSettings.videoBackend.userVisible || root._videoSettings.forceVideoDecoder.userVisible
                    || root._videoSettings.forceCpuVideoPath.userVisible)

    LabelledFactComboBox {
        label:              qsTr("Video Backend")
        fact:               root._videoSettings.videoBackend
        indexModel:         false
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }

    LabelledFactComboBox {
        objectName:         "videoDecoderPriority"
        label:              qsTr("Decoder Priority")
        fact:               root._videoSettings.forceVideoDecoder
        indexModel:         false
        visible:            fact.userVisible && root._gstreamer
        Layout.fillWidth:   true
    }

    FactCheckBoxSlider {
        objectName:         "videoForceCpuPath"
        text:               qsTr("Force CPU Video Path")
        fact:               root._videoSettings.forceCpuVideoPath
        visible:            fact.userVisible && root._gstreamer
        Layout.fillWidth:   true
    }
}
