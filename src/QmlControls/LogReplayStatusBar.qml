import QtQuick          2.3
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    height:             visible ? (rowLayout.height + (_margins * 2)) : 0
    color:              qgcPal.window

    property real   _margins:       ScreenTools.defaultFontPixelHeight / 4
    property var    _logReplayLink: null

    function pickLogFile() {
        if (mainWindow.activeVehicle) {
            mainWindow.showMessageDialog(qsTr("Log Replay"), qsTr("You must close all connections prior to replaying a log."))
            return
        }

        filePicker.openForLoad()
    }

    QGCPalette { id: qgcPal }

    QGCFileDialog {
        id:                 filePicker
        title:              qsTr("Select Telemetery Log")
        nameFilters:        [qsTr("Telemetry Logs (*.%1)").arg(QGroundControl.settingsManager.appSettings.telemetryFileExtension), qsTr("All Files (*)")]
        selectExisting:     true
        folder:             QGroundControl.settingsManager.appSettings.telemetrySavePath
        onAcceptedForLoad: {
            controller.link = QGroundControl.linkManager.startLogReplay(file)
            close()
        }
    }

    LogReplayLinkController {
        id: controller

        onPercentCompleteChanged: slider.updatePercentComplete(percentComplete)
    }

    RowLayout {
        id:                 rowLayout
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        QGCButton {
            text:       controller.isPlaying ? qsTr("Pause") : qsTr("Play")
            enabled:    controller.link
            onClicked:  controller.isPlaying = !controller.isPlaying
        }

        QGCComboBox {
            textRole:       "text"
            currentIndex:   3

            model: ListModel {
                ListElement { text: "0.1";  value: 0.1 }
                ListElement { text: "0.25"; value: 0.25 }
                ListElement { text: "0.5";  value: 0.5 }
                ListElement { text: "1x";   value: 1 }
                ListElement { text: "2x";   value: 2 }
                ListElement { text: "5x";   value: 5 }
            }

            onActivated: controller.playbackSpeed = model.get(currentIndex).value
        }

        QGCLabel { text: controller.playheadTime }

        Slider {
            id:                 slider
            Layout.fillWidth:   true
            from:               0
            to:                 100
            enabled:            controller.link

            property bool manualUpdate: false

            function updatePercentComplete(percentComplete) {
                manualUpdate = true
                value = percentComplete
                manualUpdate = false
            }

            onValueChanged: {
                if (!manualUpdate) {
                    controller.percentComplete = value
                }
            }
        }

        QGCLabel { text: controller.totalTime }

        QGCButton {
            text:       qsTr("Load Telemetry Log")
            onClicked:  pickLogFile()
            visible:    !controller.link
        }
    }
}
