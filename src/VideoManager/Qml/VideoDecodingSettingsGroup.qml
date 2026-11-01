import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Which decoder the video streams use and whether their frames reach the display through the CPU. A change restarts
// the streams. Hidden while a USB camera, which they don't apply to, is the video source.
QGCGroupBox {
    id: root

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    heading:    qsTr("Decoding")
    visible:    !QGroundControl.videoManager.isUvc
                && (root._videoSettings.forceVideoDecoder.userVisible || root._videoSettings.forceCpuVideoPath.userVisible)

    LabelledFactComboBox {
        objectName:         "videoDecoderPriority"
        label:              qsTr("Decoder Priority")
        fact:               root._videoSettings.forceVideoDecoder
        indexModel:         false
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }

    FactCheckBoxSlider {
        objectName:         "videoForceCpuPath"
        text:               qsTr("Force CPU Video Path")
        fact:               root._videoSettings.forceCpuVideoPath
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }
}
