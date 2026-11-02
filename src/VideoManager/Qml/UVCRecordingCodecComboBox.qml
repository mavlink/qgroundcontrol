import QtQuick

import QGroundControl
import QGroundControl.Controls

// Picks the codec a USB camera recording tries first, from the ones Qt can encode here. Auto, or a stored codec Qt
// can't encode here, leaves the choice to the controller: the first of its fallbacks that encodes.
LabelledComboBox {
    id: root

    readonly property Fact                  _codec:     QGroundControl.settingsManager.videoSettings.uvcRecordingCodec
    readonly property UVCCameraController   _camera:    QGroundControl.videoManager.uvcCamera
    readonly property list<string>          _codecs:    root._camera.recordingCodecs

    label:  root._codec.label
    model:  [qsTr("Auto")].concat(root._codecs.map(key => root._camera.recordingCodecName(key)))

    onActivated: (index) => {
        root._codec.rawValue = (index > 0) ? root._codecs[index - 1] : ""
    }

    // Follows the setting, also after the user picked an entry.
    Binding {
        target:     root
        property:   "currentIndex"
        value:      root._codecs.indexOf(root._codec.rawValue) + 1
    }
}
