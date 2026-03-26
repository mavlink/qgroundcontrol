import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    heading: qsTr("GCS Position")
    visible: _gcsPosition.isValid

    property var  _gcsPosition: QGroundControl.qgcPositionManger.gcsPosition
    property real _gcsHDOP:     QGroundControl.qgcPositionManger.gcsPositionHorizontalAccuracy

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
        label:     qsTr("HDOP")
        labelText: _gcsHDOP > 0 ? _gcsHDOP.toFixed(1) + " m" : qsTr("N/A")
    }
}
