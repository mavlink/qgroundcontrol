import QtQuick

import QGroundControl
import QGroundControl.Controls

// Picks the USB camera's format from the ones it offers. Auto, or a stored format this camera doesn't offer, lets Qt
// choose: the highest resolution at 30 fps or more.
LabelledComboBox {
    id: root

    readonly property Fact          _format:    QGroundControl.settingsManager.videoSettings.uvcCameraFormat
    readonly property list<string>  _formats:   QGroundControl.videoManager.uvcCamera.formats

    label:  root._format.label
    model:  [qsTr("Auto")].concat(root._formats)

    onActivated: (index) => {
        root._format.rawValue = (index > 0) ? root._formats[index - 1] : ""
    }

    // Follows the setting and the camera's formats, also after the user picked an entry.
    Binding {
        target:     root
        property:   "currentIndex"
        value:      root._formats.indexOf(root._format.rawValue) + 1
    }
}
