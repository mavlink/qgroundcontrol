/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette

Rectangle {
    id:             __mavlinkRoot
    color:          qgcPal.window
    anchors.fill:   parent

    property real _labelWidth:          ScreenTools.defaultFontPixelWidth * 28
    property real _valueWidth:          ScreenTools.defaultFontPixelWidth * 24
    property int  _selectedCount:       0
    property real _columnSpacing:       ScreenTools.defaultFontPixelHeight * 0.25
    property bool _uploadedSelected:    false
    property bool _showMavlinkLog:      QGroundControl.corePlugin.options.showPX4LogTransferOptions
    property bool _showAPMStreamRates:  QGroundControl.apmFirmwareSupported && QGroundControl.settingsManager.apmMavlinkStreamRateSettings.visible && _isAPM
    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property bool _isPX4:               _activeVehicle ? _activeVehicle.px4Firmware : true
    property bool _isAPM:               _activeVehicle ? _activeVehicle.apmFirmware : true
    property Fact _disableDataPersistenceFact: QGroundControl.settingsManager.appSettings.disableAllPersistence
    property bool _disableDataPersistence:     _disableDataPersistenceFact ? _disableDataPersistenceFact.rawValue : false
    property var  _mavlinkLogManager:   _activeVehicle ? _activeVehicle.mavlinkLogManager : null

    QGCPalette { id: qgcPal }

    Connections {
        target: _mavlinkLogManager
        onSelectedCountChanged: {
            _uploadedSelected = false
            var selected = 0
            for(var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                var logFile = _mavlinkLogManager.logFiles.get(i)
                if(logFile.selected) {
                    selected++
                    //-- If an uploaded file is selected, disable "Upload" button
                    if(logFile.uploaded) {
                        _uploadedSelected = true
                    }
                }
            }
            _selectedCount = selected
        }
    }

    function saveItems() {
        _mavlinkLogManager.videoURL = videoUrlField.text
        _mavlinkLogManager.feedback = feedbackTextArea.text
        _mavlinkLogManager.emailAddress = emailField.text
        _mavlinkLogManager.description = descField.text
        _mavlinkLogManager.uploadURL = urlField.text
        _mavlinkLogManager.emailAddress = emailField.text
        if(autoUploadCheck.checked && _mavlinkLogManager.emailAddress === "") {
            autoUploadCheck.checked = false
        } else {
            _mavlinkLogManager.enableAutoUpload = autoUploadCheck.checked
        }
    }

    MessageDialog {
        id:         emptyEmailDialog
        visible:    false
        //icon:       StandardIcon.Warning
        buttons:    MessageDialog.Close
        title:      qsTr("MAVLink Logging")
        text:       qsTr("Please enter an email address before uploading MAVLink log files.")
    }

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      settingsColumn.height
        contentWidth:       settingsColumn.width
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:                 settingsColumn
            width:              __mavlinkRoot.width
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.margins:    ScreenTools.defaultFontPixelWidth

            //-----------------------------------------------------------------
            //-- Mavlink Logging
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             mavlogLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:            _showMavlinkLog && _isPX4
                QGCLabel {
                    id:             mavlogLabel
                    text:           qsTr("MAVLink 2.0 Logging (PX4 Pro Only)")
                    font.bold:      true
                }
            }
            Rectangle {
                height:         mavlogColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        _showMavlinkLog && _isPX4
                Column {
                    id:         mavlogColumn
                    spacing:    _columnSpacing
                    anchors.centerIn: parent
                    //-----------------------------------------------------------------
                    //-- Manual Start/Stop
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Manual Start/Stop:")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCButton {
                            text:               qsTr("Start Logging")
                            width:              (_valueWidth * 0.5) - (ScreenTools.defaultFontPixelWidth * 0.5)
                            enabled:            !_mavlinkLogManager.logRunning && _mavlinkLogManager.canStartLog && !_disableDataPersistence
                            onClicked:          _mavlinkLogManager.startLogging()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCButton {
                            text:               qsTr("Stop Logging")
                            width:              (_valueWidth * 0.5) - (ScreenTools.defaultFontPixelWidth * 0.5)
                            enabled:            _mavlinkLogManager.logRunning && !_disableDataPersistence
                            onClicked:          _mavlinkLogManager.stopLogging()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Enable auto log on arming
                    QGCCheckBox {
                        text:       qsTr("Enable automatic logging")
                        checked:    _mavlinkLogManager.enableAutoStart
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            _mavlinkLogManager.enableAutoStart = checked
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Logging
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             logLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:            _showMavlinkLog && _isPX4
                QGCLabel {
                    id:             logLabel
                    text:           qsTr("MAVLink 2.0 Log Uploads (PX4 Pro Only)")
                    font.bold:      true
                }
            }
            Rectangle {
                height:         logColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        _showMavlinkLog && _isPX4
                Column {
                    id:         logColumn
                    spacing:    _columnSpacing
                    anchors.centerIn: parent
                    //-----------------------------------------------------------------
                    //-- Email address Field
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   emailField.baseline
                            text:               qsTr("Email address for Log Upload:")
                        }
                        QGCTextField {
                            id:         emailField
                            text:       _mavlinkLogManager.emailAddress
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            inputMethodHints:       Qt.ImhNoAutoUppercase | Qt.ImhEmailCharactersOnly
                            anchors.verticalCenter: parent.verticalCenter
                            onEditingFinished: {
                                saveItems();
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Description Field
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   descField.baseline
                            text:               qsTr("Default Description:")
                        }
                        QGCTextField {
                            id:         descField
                            text:       _mavlinkLogManager.description
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            anchors.verticalCenter: parent.verticalCenter
                            onEditingFinished: {
                                saveItems();
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Upload URL
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   urlField.baseline
                            text:               qsTr("Default Upload URL")
                        }
                        QGCTextField {
                            id:         urlField
                            text:       _mavlinkLogManager.uploadURL
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            inputMethodHints:       Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                            anchors.verticalCenter: parent.verticalCenter
                            onEditingFinished: {
                                saveItems();
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Video URL
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   videoUrlField.baseline
                            text:               qsTr("Video URL:")
                        }
                        QGCTextField {
                            id:         videoUrlField
                            text:       _mavlinkLogManager.videoURL
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            inputMethodHints:       Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Wind Speed
                    Row {
                        spacing:                ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   windCombo.baseline
                            text:               qsTr("Wind Speed:")
                        }
                        QGCComboBox {
                            id:         windCombo
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            textRole:   "text"
                            model: ListModel {
                                id: windItems
                                ListElement { text: qsTr("Please Select"); value: -1 }
                                ListElement { text: qsTr("Calm");     value: 0 }
                                ListElement { text: qsTr("Breeze");   value: 5 }
                                ListElement { text: qsTr("Gale");     value: 8 }
                                ListElement { text: qsTr("Storm");    value: 10 }
                            }
                            onActivated: (index) => {
                                saveItems();
                                _mavlinkLogManager.windSpeed = windItems.get(index).value
                                //console.log('Set Wind: ' + windItems.get(index).value)
                            }
                            Component.onCompleted: {
                                for(var i = 0; i < windItems.count; i++) {
                                    if(windItems.get(i).value === _mavlinkLogManager.windSpeed) {
                                        windCombo.currentIndex = i;
                                        //console.log('Wind: ' + windItems.get(i).value)
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Flight Rating
                    Row {
                        spacing:                ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   ratingCombo.baseline
                            text:               qsTr("Flight Rating:")
                        }
                        QGCComboBox {
                            id:         ratingCombo
                            width:      _valueWidth
                            enabled:    !_disableDataPersistence
                            textRole:   "text"
                            model: ListModel {
                                id: ratingItems
                                ListElement { text: qsTr("Please Select");            value: "notset"}
                                ListElement { text: qsTr("Crashed (Pilot Error)");    value: "crash_pilot" }
                                ListElement { text: qsTr("Crashed (Software or Hardware issue)");   value: "crash_sw_hw" }
                                ListElement { text: qsTr("Unsatisfactory");           value: "unsatisfactory" }
                                ListElement { text: qsTr("Good");                     value: "good" }
                                ListElement { text: qsTr("Great");                    value: "great" }
                            }
                            onActivated: (index) => {
                                saveItems();
                                _mavlinkLogManager.rating = ratingItems.get(index).value
                                //console.log('Set Rating: ' + ratingItems.get(index).value)
                            }
                            Component.onCompleted: {
                                for(var i = 0; i < ratingItems.count; i++) {
                                    if(ratingItems.get(i).value === _mavlinkLogManager.rating) {
                                        ratingCombo.currentIndex = i;
                                        //console.log('Rating: ' + ratingItems.get(i).value)
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Feedback
                    Row {
                        spacing:                ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Additional Feedback:")
                        }
                        TextArea {
                            id:                 feedbackTextArea
                            width:              _valueWidth
                            height:             ScreenTools.defaultFontPixelHeight * 4
                            font.pointSize:     ScreenTools.defaultFontPointSize
                            text:               _mavlinkLogManager.feedback
                            enabled:            !_disableDataPersistence
                            color:              qgcPal.textFieldText
                            background:         Rectangle { color: qgcPal.textField }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Public Log
                    QGCCheckBox {
                        text:       qsTr("Make this log publicly available")
                        checked:    _mavlinkLogManager.publicLog
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            _mavlinkLogManager.publicLog = checked
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Automatic Upload
                    QGCCheckBox {
                        id:         autoUploadCheck
                        text:       qsTr("Enable automatic log uploads")
                        checked:    _mavlinkLogManager.enableAutoUpload
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            saveItems();
                            if(checked && _mavlinkLogManager.emailAddress === "")
                                emptyEmailDialog.open()
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Delete log after upload
                    QGCCheckBox {
                        text:       qsTr("Delete log file after uploading")
                        checked:    _mavlinkLogManager.deleteAfterUpload
                        enabled:    autoUploadCheck.checked && !_disableDataPersistence
                        onClicked: {
                            _mavlinkLogManager.deleteAfterUpload = checked
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Log Files
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             logFilesLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:            _showMavlinkLog
                QGCLabel {
                    id:             logFilesLabel
                    text:           qsTr("Saved Log Files")
                    font.bold:      true
                }
            }
            Rectangle {
                height:         logFilesColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        _showMavlinkLog
                Column {
                    id:         logFilesColumn
                    spacing:    _columnSpacing * 4
                    anchors.centerIn: parent
                    width:          ScreenTools.defaultFontPixelWidth * 68
                    Rectangle {
                        width:          ScreenTools.defaultFontPixelWidth  * 64
                        height:         ScreenTools.defaultFontPixelHeight * 14
                        anchors.horizontalCenter: parent.horizontalCenter
                        color:          qgcPal.window
                        border.color:   qgcPal.text
                        border.width:   0.5
                        QGCListView {
                            width:          ScreenTools.defaultFontPixelWidth  * 56
                            height:         ScreenTools.defaultFontPixelHeight * 12
                            anchors.centerIn: parent
                            orientation:    ListView.Vertical
                            model:          _mavlinkLogManager.logFiles
                            clip:           true
                            delegate: Rectangle {
                                width:          ScreenTools.defaultFontPixelWidth  * 52
                                height:         selectCheck.height
                                color:          qgcPal.window
                                Row {
                                    width:  ScreenTools.defaultFontPixelWidth  * 50
                                    anchors.centerIn: parent
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    QGCCheckBox {
                                        id:         selectCheck
                                        width:      ScreenTools.defaultFontPixelWidth * 4
                                        checked:    object.selected
                                        enabled:    !object.writing && !object.uploading
                                        anchors.verticalCenter: parent.verticalCenter
                                        onClicked:  {
                                            object.selected = checked
                                        }
                                    }
                                    QGCLabel {
                                        text:       object.name
                                        width:      ScreenTools.defaultFontPixelWidth * 28
                                        color:      object.writing ? qgcPal.warningText : qgcPal.text
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCLabel {
                                        text:       Number(object.size).toLocaleString(Qt.locale(), 'f', 0)
                                        visible:    !object.uploading && !object.uploaded
                                        width:      ScreenTools.defaultFontPixelWidth * 20;
                                        color:      object.writing ? qgcPal.warningText : qgcPal.text
                                        horizontalAlignment: Text.AlignRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCLabel {
                                        text:      qsTr("Uploaded")
                                        visible:    object.uploaded
                                        width:      ScreenTools.defaultFontPixelWidth * 20;
                                        horizontalAlignment: Text.AlignRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    ProgressBar {
                                        visible:    object.uploading && !object.uploaded
                                        width:      ScreenTools.defaultFontPixelWidth * 20;
                                        height:     ScreenTools.defaultFontPixelHeight
                                        anchors.verticalCenter: parent.verticalCenter
                                        from:   0
                                        to:   100
                                        value:          object.progress * 100.0
                                    }
                                }
                            }
                        }
                    }
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:      qsTr("Check All")
                            enabled:    !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
                            onClicked: {
                                for(var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                                    var logFile = _mavlinkLogManager.logFiles.get(i)
                                    logFile.selected = true
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Check None")
                            enabled:    !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
                            onClicked: {
                                for(var i = 0; i < _mavlinkLogManager.logFiles.count; i++) {
                                    var logFile = _mavlinkLogManager.logFiles.get(i)
                                    logFile.selected = false
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Delete Selected")
                            enabled:    _selectedCount > 0 && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
                            onClicked:  deleteDialog.open()
                            MessageDialog {
                                id:         deleteDialog
                                visible:    false
                                //icon:       StandardIcon.Warning
                                buttons:    MessageDialog.Yes | MessageDialog.No
                                title:      qsTr("Delete Selected Log Files")
                                text:       qsTr("Confirm deleting selected log files?")
                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _mavlinkLogManager.deleteLog()
                                        break;
                                    }
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Upload Selected")
                            enabled:    _selectedCount > 0 && !_mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning && !_uploadedSelected
                            visible:    !_mavlinkLogManager.uploading
                            onClicked:  {
                                saveItems();
                                if(_mavlinkLogManager.emailAddress === "")
                                    emptyEmailDialog.open()
                                else
                                    uploadDialog.open()
                            }
                            MessageDialog {
                                id:         uploadDialog
                                visible:    false
                                //icon:       StandardIcon.Question
                                buttons:    MessageDialog.Yes | MessageDialog.No
                                title:      qsTr("Upload Selected Log Files")
                                text:       qsTr("Confirm uploading selected log files?")
                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _mavlinkLogManager.uploadLog()
                                        break;
                                    }
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Cancel")
                            enabled:    _mavlinkLogManager.uploading && !_mavlinkLogManager.logRunning
                            visible:    _mavlinkLogManager.uploading
                            onClicked:  cancelDialog.open()
                            MessageDialog {
                                id:         cancelDialog
                                visible:    false
                                //icon:       StandardIcon.Warning
                                buttons:    MessageDialog.Yes | MessageDialog.No
                                title:      qsTr("Cancel Upload")
                                text:       qsTr("Confirm canceling the upload process?")
                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _mavlinkLogManager.cancelUpload()
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
