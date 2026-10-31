import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// How the video is drawn: the grid lines over it and how it fits the view.
QGCGroupBox {
    id:         root
    heading:    qsTr("Video Display")

    readonly property VideoSettings _videoSettings: QGroundControl.settingsManager.videoSettings

    FactCheckBoxSlider {
        Layout.fillWidth:   true
        text:               qsTr("Grid Lines")
        fact:               root._videoSettings.gridLines
        visible:            fact.userVisible
    }

    LabelledFactComboBox {
        Layout.fillWidth:   true
        label:              qsTr("Display Fit")
        fact:               root._videoSettings.videoFit
        indexModel:         false
        visible:            fact.userVisible
    }
}
