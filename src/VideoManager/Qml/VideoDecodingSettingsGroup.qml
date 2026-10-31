import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Which decoder the video streams use and whether their frames reach the display through the CPU. A change restarts
// the streams.
QGCGroupBox {
    id:         root
    heading:    qsTr("Decoding")
    visible:    _videoSettings.forceVideoDecoder.userVisible || _videoSettings.forceCpuVideoPath.userVisible

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    LabelledFactComboBox {
        objectName:         "videoDecoderPriority"
        Layout.fillWidth:   true
        label:              qsTr("Decoder Priority")
        fact:               root._videoSettings.forceVideoDecoder
        indexModel:         false
        visible:            fact.userVisible
    }

    FactCheckBoxSlider {
        objectName:         "videoForceCpuPath"
        Layout.fillWidth:   true
        text:               qsTr("Force CPU Video Path")
        fact:               root._videoSettings.forceCpuVideoPath
        visible:            fact.userVisible
    }
}
