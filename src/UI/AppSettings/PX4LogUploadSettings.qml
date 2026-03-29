import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    Layout.fillWidth: true
    heading:          qsTr("MAVLink 2.0 Log Uploads (PX4 Pro Only)")
    visible:          QGroundControl.corePlugin.options.showPX4LogTransferOptions && _isPX4

    property var  _activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property bool _isPX4:                  _activeVehicle ? _activeVehicle.px4Firmware : true
    property var  _mavlinkLogManager:      _activeVehicle ? _activeVehicle.mavlinkLogManager : null
    property bool _disableDataPersistence: QGroundControl.settingsManager.appSettings.disableAllPersistence.rawValue
    property real _valueWidth:             ScreenTools.defaultFontPixelWidth * 24

    function saveItems() {
        if (!_mavlinkLogManager) return
        _mavlinkLogManager.videoURL      = videoUrlField.text
        _mavlinkLogManager.feedback      = feedbackTextArea.text
        _mavlinkLogManager.emailAddress  = emailField.text
        _mavlinkLogManager.description   = descField.text
        _mavlinkLogManager.uploadURL     = urlField.text
        if (autoUploadCheck.checked && _mavlinkLogManager.emailAddress === "") {
            autoUploadCheck.checked = false
            _mavlinkLogManager.enableAutoUpload = false
        } else {
            _mavlinkLogManager.enableAutoUpload = autoUploadCheck.checked
        }
    }

    MessageDialog {
        id:      emptyEmailDialog
        visible: false
        buttons: MessageDialog.Close
        title:   qsTr("MAVLink Logging")
        text:    qsTr("Please enter an email address before uploading MAVLink log files.")
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:             qsTr("Email address for Log Upload:")
            Layout.fillWidth: true
        }

        QGCTextField {
            id:                emailField
            Layout.fillWidth:  true
            text:              _mavlinkLogManager ? _mavlinkLogManager.emailAddress : ""
            enabled:           !_disableDataPersistence
            inputMethodHints:  Qt.ImhNoAutoUppercase | Qt.ImhEmailCharactersOnly
            onEditingFinished: saveItems()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:             qsTr("Default Description:")
            Layout.fillWidth: true
        }

        QGCTextField {
            id:                descField
            Layout.fillWidth:  true
            text:              _mavlinkLogManager ? _mavlinkLogManager.description : ""
            enabled:           !_disableDataPersistence
            onEditingFinished: saveItems()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:             qsTr("Default Upload URL:")
            Layout.fillWidth: true
        }

        QGCTextField {
            id:                urlField
            Layout.fillWidth:  true
            text:              _mavlinkLogManager ? _mavlinkLogManager.uploadURL : ""
            enabled:           !_disableDataPersistence
            inputMethodHints:  Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
            onEditingFinished: saveItems()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:             qsTr("Video URL:")
            Layout.fillWidth: true
        }

        QGCTextField {
            id:                videoUrlField
            Layout.fillWidth:  true
            text:              _mavlinkLogManager ? _mavlinkLogManager.videoURL : ""
            enabled:           !_disableDataPersistence
            inputMethodHints:  Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
        }
    }

    LabelledComboBox {
        Layout.fillWidth: true
        label:            qsTr("Wind Speed")
        enabled:          !_disableDataPersistence
        model:            [qsTr("Please Select"), qsTr("Calm"), qsTr("Breeze"), qsTr("Gale"), qsTr("Storm")]

        property var _windValues: [-1, 0, 5, 8, 10]

        onActivated: (index) => {
            saveItems()
            if (_mavlinkLogManager) _mavlinkLogManager.windSpeed = _windValues[index]
        }
        Component.onCompleted: {
            if (!_mavlinkLogManager) return
            for (var i = 0; i < _windValues.length; i++) {
                if (_windValues[i] === _mavlinkLogManager.windSpeed) {
                    comboBox.currentIndex = i
                    break
                }
            }
        }
    }

    LabelledComboBox {
        Layout.fillWidth: true
        label:            qsTr("Flight Rating")
        enabled:          !_disableDataPersistence
        model:            [qsTr("Please Select"), qsTr("Crashed (Pilot Error)"), qsTr("Crashed (Software or Hardware issue)"),
                           qsTr("Unsatisfactory"), qsTr("Good"), qsTr("Great")]

        property var _ratingValues: ["notset", "crash_pilot", "crash_sw_hw", "unsatisfactory", "good", "great"]

        onActivated: (index) => {
            saveItems()
            if (_mavlinkLogManager) _mavlinkLogManager.rating = _ratingValues[index]
        }
        Component.onCompleted: {
            if (!_mavlinkLogManager) return
            for (var i = 0; i < _ratingValues.length; i++) {
                if (_ratingValues[i] === _mavlinkLogManager.rating) {
                    comboBox.currentIndex = i
                    break
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing:          ScreenTools.defaultFontPixelWidth

        QGCLabel {
            text:             qsTr("Additional Feedback:")
            Layout.alignment: Qt.AlignTop
        }

        TextArea {
            id:               feedbackTextArea
            Layout.fillWidth: true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 4
            font.pointSize:   ScreenTools.defaultFontPointSize
            text:             _mavlinkLogManager ? _mavlinkLogManager.feedback : ""
            enabled:          !_disableDataPersistence
            color:            qgcPal.textFieldText
            background:       Rectangle { color: qgcPal.textField }
        }
    }

    QGCPalette { id: qgcPal }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("Make this log publicly available")
        checked: _mavlinkLogManager ? _mavlinkLogManager.publicLog : false
        enabled: !_disableDataPersistence
        onClicked: _mavlinkLogManager.publicLog = checked
    }

    QGCCheckBoxSlider {
        id:               autoUploadCheck
        Layout.fillWidth: true
        text:    qsTr("Enable automatic log uploads")
        checked: _mavlinkLogManager ? _mavlinkLogManager.enableAutoUpload : false
        enabled: !_disableDataPersistence
        onClicked: {
            const wantsAutoUpload = checked
            saveItems()
            if (wantsAutoUpload && _mavlinkLogManager && _mavlinkLogManager.emailAddress === "")
                emptyEmailDialog.open()
        }
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text:    qsTr("Delete log file after uploading")
        checked: _mavlinkLogManager ? _mavlinkLogManager.deleteAfterUpload : false
        enabled: autoUploadCheck.checked && !_disableDataPersistence
        onClicked: _mavlinkLogManager.deleteAfterUpload = checked
    }
}
