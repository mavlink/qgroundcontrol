import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// How the video is drawn: the grid lines and telemetry over it, and how it fits the view.
QGCGroupBox {
    id: root

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    heading: qsTr("Video Display")

    FactCheckBoxSlider {
        text:               qsTr("Grid Lines")
        fact:               root._videoSettings.gridLines
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }

    FactCheckBoxSlider {
        text:               qsTr("Telemetry Overlay")
        fact:               root._videoSettings.showTelemetryOverlay
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }

    LabelledFactComboBox {
        label:              qsTr("Display Fit")
        fact:               root._videoSettings.videoFit
        indexModel:         false
        visible:            fact.userVisible
        Layout.fillWidth:   true
    }
}
