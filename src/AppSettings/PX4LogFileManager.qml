import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    Layout.fillWidth: true
    heading:          qsTr("Saved Log Files")
    visible:          QGroundControl.corePlugin.options.showPX4LogTransferOptions

    property var  _activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property var  _mavlinkLogManager:      _activeVehicle ? _activeVehicle.mavlinkLogManager : null
    property bool _disableDataPersistence: QGroundControl.settingsManager.appSettings.disableAllPersistence.rawValue
    property int  _selectedCount:          0
    property bool _uploadedSelected:       false

    Connections {
        target: _mavlinkLogManager
        function onSelectedCountChanged() {
            _uploadedSelected = false
            var selected = 0
            for (var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                var logFile = _mavlinkLogManager.logFiles.get(i)
                if (logFile.selected) {
                    selected++
                    if (logFile.uploaded) {
                        _uploadedSelected = true
                    }
                }
            }
            _selectedCount = selected
        }
    }

    QGCPalette { id: qgcPal }

    Rectangle {
        Layout.fillWidth:       true
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 14
        color:                  qgcPal.window
        border.color:           qgcPal.text
        border.width:           0.5

        QGCListView {
            anchors.fill:    parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            orientation:     ListView.Vertical
            model:           _mavlinkLogManager ? _mavlinkLogManager.logFiles : undefined
            clip:            true

            delegate: Rectangle {
                width:  parent ? parent.width : 0
                height: selectCheck.height
                color:  qgcPal.window

                RowLayout {
                    anchors.fill: parent
                    spacing:      ScreenTools.defaultFontPixelWidth

                    QGCCheckBox {
                        id:      selectCheck
                        checked: object.selected
                        enabled: !object.writing && !object.uploading
                        onClicked: object.selected = checked
                    }

                    QGCLabel {
                        text:             object.name
                        Layout.fillWidth: true
                        color:            object.writing ? qgcPal.warningText : qgcPal.text
                    }

                    QGCLabel {
                        text:    Number(object.size).toLocaleString(Qt.locale(), 'f', 0)
                        visible: !object.uploading && !object.uploaded
                        color:   object.writing ? qgcPal.warningText : qgcPal.text
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12
                    }

                    QGCLabel {
                        text:    qsTr("Uploaded")
                        visible: object.uploaded
                        horizontalAlignment: Text.AlignRight
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12
                    }

                    ProgressBar {
                        visible:              object.uploading && !object.uploaded
                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 12
                        height:               ScreenTools.defaultFontPixelHeight
                        from:                 0
                        to:                   100
                        value:                object.progress * 100.0
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCButton {
            text:    qsTr("Check All")
            enabled: _mavlinkLogManager && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
            onClicked: {
                for (var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                    _mavlinkLogManager.logFiles.get(i).selected = true
                }
            }
        }

        QGCButton {
            text:    qsTr("Check None")
            enabled: _mavlinkLogManager && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
            onClicked: {
                for (var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                    _mavlinkLogManager.logFiles.get(i).selected = false
                }
            }
        }

        QGCButton {
            text:    qsTr("Delete Selected")
            enabled: _selectedCount > 0 && _mavlinkLogManager && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
            onClicked: deleteDialog.open()

            MessageDialog {
                id:      deleteDialog
                visible: false
                buttons: MessageDialog.Yes | MessageDialog.No
                title:   qsTr("Delete Selected Log Files")
                text:    qsTr("Confirm deleting selected log files?")
                onButtonClicked: function (button, role) {
                    if (button === MessageDialog.Yes)
                        _mavlinkLogManager.deleteLog()
                }
            }
        }

        QGCButton {
            text:    qsTr("Upload Selected")
            enabled: _selectedCount > 0 && _mavlinkLogManager && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning && !_uploadedSelected
            visible: !_mavlinkLogManager || !_mavlinkLogManager.uploading
            onClicked: {
                if (_mavlinkLogManager.emailAddress === "")
                    emptyEmailDialog.open()
                else
                    uploadDialog.open()
            }

            MessageDialog {
                id:      emptyEmailDialog
                visible: false
                buttons: MessageDialog.Close
                title:   qsTr("MAVLink Logging")
                text:    qsTr("Please enter an email address before uploading MAVLink log files.")
            }

            MessageDialog {
                id:      uploadDialog
                visible: false
                buttons: MessageDialog.Yes | MessageDialog.No
                title:   qsTr("Upload Selected Log Files")
                text:    qsTr("Confirm uploading selected log files?")
                onButtonClicked: function (button, role) {
                    if (button === MessageDialog.Yes)
                        _mavlinkLogManager.uploadLog()
                }
            }
        }

        QGCButton {
            text:    qsTr("Cancel")
            enabled: _mavlinkLogManager && _mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
            visible: _mavlinkLogManager && _mavlinkLogManager.uploading
            onClicked: cancelDialog.open()

            MessageDialog {
                id:      cancelDialog
                visible: false
                buttons: MessageDialog.Yes | MessageDialog.No
                title:   qsTr("Cancel Upload")
                text:    qsTr("Confirm canceling the upload process?")
                onButtonClicked: function (button, role) {
                    if (button === MessageDialog.Yes)
                        _mavlinkLogManager.cancelUpload()
                }
            }
        }
    }
}
