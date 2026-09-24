import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    heading: qsTr("GCS Position")
    visible: _gcsPosition.isValid

    property var  _gcsPosition: QGroundControl.positionManager.gcsPosition
    property real _horizontalAccuracy: QGroundControl.positionManager.gcsPositionHorizontalAccuracy

    LabelledLabel {
        Layout.fillWidth: true
        label:     qsTr("Latitude")
        labelText: _gcsPosition.isValid ? _gcsPosition.latitude.toFixed(7) : qsTr("N/A")
    }

    LabelledLabel {
        Layout.fillWidth: true
        label:     qsTr("Longitude")
        labelText: _gcsPosition.isValid ? _gcsPosition.longitude.toFixed(7) : qsTr("N/A")
    }

    LabelledLabel {
        Layout.fillWidth: true
        objectName: "gcsHorizontalAccuracy"
        label:     qsTr("Horizontal accuracy")
        labelText: Number.isFinite(_horizontalAccuracy) && _horizontalAccuracy >= 0
                   ? qsTr("%1 m").arg(_horizontalAccuracy.toFixed(1)) : qsTr("N/A")
    }
}
