/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:             __mavlinkRoot
    color:          qgcPal.window
    anchors.fill:   parent

    property real _labelWidth:          ScreenTools.defaultFontPixelWidth * 28
    property real _valueWidth:          ScreenTools.defaultFontPixelWidth * 24
    property int  _selectedCount:       0
    property real _columnSpacing:       ScreenTools.defaultFontPixelHeight * 0.25
    property bool _uploadedSelected:    false
    property bool _showMavlinkLog:      QGroundControl.corePlugin.options.showMavlinkLogOptions
    property bool _showAPMStreamRates:  QGroundControl.apmFirmwareSupported && QGroundControl.settingsManager.apmMavlinkStreamRateSettings.visible
    property Fact _disableDataPersistenceFact: QGroundControl.settingsManager.appSettings.disableAllPersistence
    property bool _disableDataPersistence:     _disableDataPersistenceFact ? _disableDataPersistenceFact.rawValue : false

    QGCPalette { id: qgcPal }

    Connections {
        target: QGroundControl.mavlinkLogManager
        onSelectedCountChanged: {
            _uploadedSelected = false
            var selected = 0
            for(var i = 0; i < QGroundControl.mavlinkLogManager.logFiles.count; i++) {
                var logFile = QGroundControl.mavlinkLogManager.logFiles.get(i)
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

    function saveItems()
    {
        QGroundControl.mavlinkSystemID = parseInt(sysidField.text)
        QGroundControl.mavlinkLogManager.videoURL = videoUrlField.text
        QGroundControl.mavlinkLogManager.feedback = feedbackTextArea.text
        QGroundControl.mavlinkLogManager.emailAddress = emailField.text
        QGroundControl.mavlinkLogManager.description = descField.text
        QGroundControl.mavlinkLogManager.uploadURL = urlField.text
        QGroundControl.mavlinkLogManager.emailAddress = emailField.text
        if(autoUploadCheck.checked && QGroundControl.mavlinkLogManager.emailAddress === "") {
            autoUploadCheck.checked = false
        } else {
            QGroundControl.mavlinkLogManager.enableAutoUpload = autoUploadCheck.checked
        }
    }

    MessageDialog {
        id:         emptyEmailDialog
        visible:    false
        icon:       StandardIcon.Warning
        standardButtons: StandardButton.Close
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
            //-- Ground Station
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             gcsLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    id:             gcsLabel
                    text:           qsTr("Ground Station")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:         gcsColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                Column {
                    id:         gcsColumn
                    spacing:    _columnSpacing
                    anchors.centerIn: parent
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            width:              _labelWidth
                            anchors.baseline:   sysidField.baseline
                            text:               qsTr("MAVLink System ID:")
                        }
                        QGCTextField {
                            id:     sysidField
                            text:   QGroundControl.mavlinkSystemID.toString()
                            width:  _valueWidth
                            inputMethodHints:       Qt.ImhFormattedNumbersOnly
                            anchors.verticalCenter: parent.verticalCenter
                            onEditingFinished: {
                                saveItems();
                            }
                        }
                    }

                    QGCCheckBox {
                        text:       qsTr("Emit heartbeat")
                        checked:    QGroundControl.multiVehicleManager.gcsHeartBeatEnabled
                        onClicked: {
                            QGroundControl.multiVehicleManager.gcsHeartBeatEnabled = checked
                        }
                    }

                    QGCCheckBox {
                        text:       qsTr("Only accept MAVs with same protocol version")
                        checked:    QGroundControl.isVersionCheckEnabled
                        onClicked: {
                            QGroundControl.isVersionCheckEnabled = checked
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Stream Rates
            Item {
                id:                         apmStreamRatesLabel
                width:                      __mavlinkRoot.width * 0.8
                height:                     streamRatesLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    _showAPMStreamRates
                QGCLabel {
                    id:             streamRatesLabel
                    text:           qsTr("Telemetry Stream Rates (ArduPilot Only)")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:                     streamRatesColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:                      __mavlinkRoot.width * 0.8
                color:                      qgcPal.windowShade
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    _showAPMStreamRates

                ColumnLayout {
                    id:                 streamRatesColumn
                    spacing:            ScreenTools.defaultFontPixelHeight / 2
                    anchors.centerIn:   parent

                    property bool allStreamsControlledByVehicle: !QGroundControl.settingsManager.appSettings.apmStartMavlinkStreams.rawValue

                    QGCCheckBox {
                        text:               qsTr("All Streams Controlled By Vehicle Settings")
                        checked:            streamRatesColumn.allStreamsControlledByVehicle
                        onClicked:          QGroundControl.settingsManager.appSettings.apmStartMavlinkStreams.rawValue = !checked
                    }

                    GridLayout {
                        columns:    2
                        enabled:    !streamRatesColumn.allStreamsControlledByVehicle

                        QGCLabel { text:  qsTr("Raw Sensors") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateRawSensors : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("Extended Status") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateExtendedStatus : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("RC Channel") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateRCChannels : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("Position") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRatePosition : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("Extra 1") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateExtra1 : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("Extra 2") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateExtra2 : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }

                        QGCLabel { text:  qsTr("Extra 3") }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.apmMavlinkStreamRateSettings ? QGroundControl.settingsManager.apmMavlinkStreamRateSettings.streamRateExtra3 : null
                            indexModel:             false
                            Layout.preferredWidth:  _valueWidth
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Status
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             mavStatusLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    id:             mavStatusLabel
                    text:           qsTr("MAVLink Link Status (Current Vehicle)")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:         mavStatusColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                Column {
                    id:         mavStatusColumn
                    width:      gcsColumn.width
                    spacing:    _columnSpacing
                    anchors.centerIn: parent
                    //-----------------------------------------------------------------
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Total messages sent (computed):")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            width:              _valueWidth
                            text:               activeVehicle ? activeVehicle.mavlinkSentCount : qsTr("Not Connected")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Total messages received:")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            width:              _valueWidth
                            text:               activeVehicle ? activeVehicle.mavlinkReceivedCount : qsTr("Not Connected")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Total message loss:")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            width:              _valueWidth
                            text:               activeVehicle ? activeVehicle.mavlinkLossCount : qsTr("Not Connected")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            width:              _labelWidth
                            text:               qsTr("Loss rate:")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            width:              _valueWidth
                            text:               activeVehicle ? activeVehicle.mavlinkLossPercent.toFixed(0) + '%' : qsTr("Not Connected")
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Mavlink Logging
            Item {
                width:              __mavlinkRoot.width * 0.8
                height:             mavlogLabel.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:            _showMavlinkLog
                QGCLabel {
                    id:             mavlogLabel
                    text:           qsTr("MAVLink 2.0 Logging (PX4 Pro Only)")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:         mavlogColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        _showMavlinkLog
                Column {
                    id:         mavlogColumn
                    width:      gcsColumn.width
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
                            enabled:            !QGroundControl.mavlinkLogManager.logRunning && QGroundControl.mavlinkLogManager.canStartLog && !_disableDataPersistence
                            onClicked:          QGroundControl.mavlinkLogManager.startLogging()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCButton {
                            text:               qsTr("Stop Logging")
                            width:              (_valueWidth * 0.5) - (ScreenTools.defaultFontPixelWidth * 0.5)
                            enabled:            QGroundControl.mavlinkLogManager.logRunning && !_disableDataPersistence
                            onClicked:          QGroundControl.mavlinkLogManager.stopLogging()
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Enable auto log on arming
                    QGCCheckBox {
                        text:       qsTr("Enable automatic logging")
                        checked:    QGroundControl.mavlinkLogManager.enableAutoStart
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            QGroundControl.mavlinkLogManager.enableAutoStart = checked
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
                visible:            _showMavlinkLog
                QGCLabel {
                    id:             logLabel
                    text:           qsTr("MAVLink 2.0 Log Uploads (PX4 Pro Only)")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:         logColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:          __mavlinkRoot.width * 0.8
                color:          qgcPal.windowShade
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter: parent.horizontalCenter
                visible:        _showMavlinkLog
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
                            text:       QGroundControl.mavlinkLogManager.emailAddress
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
                            text:       QGroundControl.mavlinkLogManager.description
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
                            text:       QGroundControl.mavlinkLogManager.uploadURL
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
                            text:       QGroundControl.mavlinkLogManager.videoURL
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
                                ListElement { text: "Please Select"; value: -1 }
                                ListElement { text: "Calm";     value: 0 }
                                ListElement { text: "Breeze";   value: 5 }
                                ListElement { text: "Gale";     value: 8 }
                                ListElement { text: "Storm";    value: 10 }
                            }
                            onActivated: {
                                saveItems();
                                QGroundControl.mavlinkLogManager.windSpeed = windItems.get(index).value
                                //console.log('Set Wind: ' + windItems.get(index).value)
                            }
                            Component.onCompleted: {
                                for(var i = 0; i < windItems.count; i++) {
                                    if(windItems.get(i).value === QGroundControl.mavlinkLogManager.windSpeed) {
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
                                ListElement { text: "Please Select";            value: "notset"}
                                ListElement { text: "Crashed (Pilot Error)";    value: "crash_pilot" }
                                ListElement { text: "Crashed (Software or Hardware issue)";   value: "crash_sw_hw" }
                                ListElement { text: "Unsatisfactory";           value: "unsatisfactory" }
                                ListElement { text: "Good";                     value: "good" }
                                ListElement { text: "Great";                    value: "great" }
                            }
                            onActivated: {
                                saveItems();
                                QGroundControl.mavlinkLogManager.rating = ratingItems.get(index).value
                                //console.log('Set Rating: ' + ratingItems.get(index).value)
                            }
                            Component.onCompleted: {
                                for(var i = 0; i < ratingItems.count; i++) {
                                    if(ratingItems.get(i).value === QGroundControl.mavlinkLogManager.rating) {
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
                            frameVisible:       false
                            font.pointSize:     ScreenTools.defaultFontPointSize
                            text:               QGroundControl.mavlinkLogManager.feedback
                            enabled:            !_disableDataPersistence
                            style: TextAreaStyle {
                                textColor:          qgcPal.windowShade
                                backgroundColor:    qgcPal.text
                            }
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Public Log
                    QGCCheckBox {
                        text:       qsTr("Make this log publicly available")
                        checked:    QGroundControl.mavlinkLogManager.publicLog
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            QGroundControl.mavlinkLogManager.publicLog = checked
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Automatic Upload
                    QGCCheckBox {
                        id:         autoUploadCheck
                        text:       qsTr("Enable automatic log uploads")
                        checked:    QGroundControl.mavlinkLogManager.enableAutoUpload
                        enabled:    !_disableDataPersistence
                        onClicked: {
                            saveItems();
                            if(checked && QGroundControl.mavlinkLogManager.emailAddress === "")
                                emptyEmailDialog.open()
                        }
                    }
                    //-----------------------------------------------------------------
                    //-- Delete log after upload
                    QGCCheckBox {
                        text:       qsTr("Delete log file after uploading")
                        checked:    QGroundControl.mavlinkLogManager.deleteAfterUpload
                        enabled:    autoUploadCheck.checked && !_disableDataPersistence
                        onClicked: {
                            QGroundControl.mavlinkLogManager.deleteAfterUpload = checked
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
                    font.family:    ScreenTools.demiboldFontFamily
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
                            model:          QGroundControl.mavlinkLogManager.logFiles
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
                                        minimumValue:   0
                                        maximumValue:   100
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
                            enabled:    !QGroundControl.mavlinkLogManager.uploading && !QGroundControl.mavlinkLogManager.logRunning
                            onClicked: {
                                for(var i = 0; i < QGroundControl.mavlinkLogManager.logFiles.count; i++) {
                                    var logFile = QGroundControl.mavlinkLogManager.logFiles.get(i)
                                    logFile.selected = true
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Check None")
                            enabled:    !QGroundControl.mavlinkLogManager.uploading && !QGroundControl.mavlinkLogManager.logRunning
                            onClicked: {
                                for(var i = 0; i < QGroundControl.mavlinkLogManager.logFiles.count; i++) {
                                    var logFile = QGroundControl.mavlinkLogManager.logFiles.get(i)
                                    logFile.selected = false
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Delete Selected")
                            enabled:    _selectedCount > 0 && !QGroundControl.mavlinkLogManager.uploading && !QGroundControl.mavlinkLogManager.logRunning
                            onClicked:  deleteDialog.open()
                            MessageDialog {
                                id:         deleteDialog
                                visible:    false
                                icon:       StandardIcon.Warning
                                standardButtons: StandardButton.Yes | StandardButton.No
                                title:      qsTr("Delete Selected Log Files")
                                text:       qsTr("Confirm deleting selected log files?")
                                onYes: {
                                    QGroundControl.mavlinkLogManager.deleteLog()
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Upload Selected")
                            enabled:    _selectedCount > 0 && !QGroundControl.mavlinkLogManager.uploading && !QGroundControl.mavlinkLogManager.logRunning && !_uploadedSelected
                            visible:    !QGroundControl.mavlinkLogManager.uploading
                            onClicked:  {
                                saveItems();
                                if(QGroundControl.mavlinkLogManager.emailAddress === "")
                                    emptyEmailDialog.open()
                                else
                                    uploadDialog.open()
                            }
                            MessageDialog {
                                id:         uploadDialog
                                visible:    false
                                icon:       StandardIcon.Question
                                standardButtons: StandardButton.Yes | StandardButton.No
                                title:      qsTr("Upload Selected Log Files")
                                text:       qsTr("Confirm uploading selected log files?")
                                onYes: {
                                    QGroundControl.mavlinkLogManager.uploadLog()
                                }
                            }
                        }
                        QGCButton {
                            text:      qsTr("Cancel")
                            enabled:    QGroundControl.mavlinkLogManager.uploading && !QGroundControl.mavlinkLogManager.logRunning
                            visible:    QGroundControl.mavlinkLogManager.uploading
                            onClicked:  cancelDialog.open()
                            MessageDialog {
                                id:         cancelDialog
                                visible:    false
                                icon:       StandardIcon.Warning
                                standardButtons: StandardButton.Yes | StandardButton.No
                                title:      qsTr("Cancel Upload")
                                text:       qsTr("Confirm canceling the upload process?")
                                onYes: {
                                    QGroundControl.mavlinkLogManager.cancelUpload()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
