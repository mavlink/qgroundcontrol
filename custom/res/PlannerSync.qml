/*!
 * @file
 * @brief Desktop/ST16 Sync
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0
import Qt.labs.platform         1.0

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.QGCMapEngineManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Vehicle               1.0

import TyphoonHQuickInterface               1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property int    _clientCount:       TyphoonHQuickInterface.desktopSync.remoteList.length
    property real   _buttonWidth:       ScreenTools.defaultFontPixelWidth * 20
    property real   _gridElemWidth:     ScreenTools.defaultFontPixelWidth * 20
    property string _currentNode:       _clientCount && remoteCombo.currentIndex >= 0 && remoteCombo.currentIndex < _clientCount ? TyphoonHQuickInterface.desktopSync.remoteList[remoteCombo.currentIndex] : ""
    property string _dialogTitle:       ""
    property bool   _sendMission:       true
    property bool   _hasLogs:           connected ? TyphoonHQuickInterface.desktopSync.logController.fileList.length > 0 : false
    property bool   _hasMaps:           connected ? TyphoonHQuickInterface.desktopSync.mapController.fileList.length > 0 : false

    readonly property string kCancel:        qsTr("Cancel")
    readonly property string kClose:         qsTr("Close")
    readonly property string kFetchMissions: qsTr("Fetch Missions")
    readonly property string kSelectAll:     qsTr("Select All")
    readonly property string kSelectNone:    qsTr("Select None")
    readonly property string kSendMission:   qsTr("Send Missions")
    readonly property string kStart:         qsTr("Start")

    property bool   connected:          !_activeVehicle && TyphoonHQuickInterface.desktopSync.remoteReady

    onConnectedChanged: {
        if(!connected) {
            rootLoader.sourceComponent = null
            mainWindow.enableToolbar()
            TyphoonHQuickInterface.desktopSync.disconnectRemote()
        }
    }

    QGCPalette      { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCLabel {
            text:           qsTr("Cannot sync while connected to vehicle")
            font.family:    ScreenTools.demiboldFontFamily
            font.pointSize: ScreenTools.mediumFontPointSize
            visible:        _activeVehicle
            anchors.centerIn: parent
        }
        Column {
            spacing:        ScreenTools.defaultFontPixelHeight * 2
            visible:        !_activeVehicle && !_clientCount
            anchors.centerIn: parent
            QGCLabel {
                id:             noRemoteLabel
                text:           qsTr("No remotes detected")
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.mediumFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("Start your ST16 remote, make sure it is connected to the same WiFi network as this computer and start DataPilot")
                font.family:    ScreenTools.demiboldFontFamily
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
        Rectangle {
            id:             logRect
            width:          connectCol.width  + (ScreenTools.defaultFontPixelWidth  * 16)
            height:         connectCol.height + (ScreenTools.defaultFontPixelHeight * 8)
            visible:        !_activeVehicle && _clientCount && !TyphoonHQuickInterface.desktopSync.remoteReady
            color:          qgcPal.alertBackground
            border.color:   qgcPal.alertBorder
            border.width:   2
            radius:         4
            anchors.centerIn: parent
            Column {
                id:             connectCol
                spacing:        ScreenTools.defaultFontPixelHeight
                anchors.centerIn: parent
                QGCLabel {
                    text:           qsTr("Select Remote")
                    color:          qgcPal.alertText
                    font.family:    ScreenTools.demiboldFontFamily
                    font.pointSize: ScreenTools.mediumFontPointSize
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCComboBox {
                    id:             remoteCombo
                    model:          TyphoonHQuickInterface.desktopSync.remoteList
                    width:          _buttonWidth
                    currentIndex:   -1
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                QGCButton {
                    text:           qsTr("Connect")
                    width:          _buttonWidth
                    primary:        true
                    enabled:        remoteCombo.currentIndex >= 0 && remoteCombo.currentIndex < _clientCount
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        TyphoonHQuickInterface.desktopSync.connectToRemote(_currentNode)
                    }
                }
            }
        }
        Column {
            anchors.centerIn: parent
            spacing:        ScreenTools.defaultFontPixelHeight
            visible:        connected
            onVisibleChanged: {
                if(visible) {
                    TyphoonHQuickInterface.desktopSync.initLogFetch()
                    TyphoonHQuickInterface.desktopSync.initMapFetch()
                }
            }
            Rectangle {
                width:  syncGrid.width
                height: 1
                color:  qgcPal.colorGrey
            }
            GridLayout {
                id:             syncGrid
                columns:        2
                columnSpacing:  ScreenTools.defaultFontPixelWidth  * 8
                rowSpacing:     ScreenTools.defaultFontPixelHeight * 0.5
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel { text: qsTr("Local"); font.family: ScreenTools.demiboldFontFamily; Layout.alignment: Qt.AlignHCenter; }
                QGCLabel { text: TyphoonHQuickInterface.desktopSync.currentRemote; font.family: ScreenTools.demiboldFontFamily; Layout.alignment: Qt.AlignHCenter; }
                QGCButton {
                    text:           kSendMission
                    width:          _gridElemWidth
                    onClicked: {
                        _sendMission = true
                        _dialogTitle = kSendMission + qsTr(" To ") + TyphoonHQuickInterface.desktopSync.currentRemote
                        rootLoader.sourceComponent = syncFilesDialog
                        mainWindow.disableToolbar()
                    }
                }
                QGCButton {
                    text:               kFetchMissions
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {
                        _sendMission = false
                        _dialogTitle = kFetchMissions + qsTr(" From ") + TyphoonHQuickInterface.desktopSync.currentRemote
                        rootLoader.sourceComponent = syncFilesDialog
                        mainWindow.disableToolbar()
                    }
                }
                QGCButton {
                    text:               qsTr("Send Maps")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {
                        rootLoader.sourceComponent = uploadMapsDlg
                        mainWindow.disableToolbar()
                    }
                }
                QGCButton {
                    text:               qsTr("Fetch Maps")
                    width:              _gridElemWidth
                    enabled:            _hasMaps
                    Layout.fillWidth:   true
                    onClicked: {
                        rootLoader.sourceComponent = fetchMapsDialog
                        mainWindow.disableToolbar()
                    }
                }
                Item {
                    height:             1
                    width:              1
                }
                QGCButton {
                    text:               qsTr("Fetch Logs")
                    width:              _gridElemWidth
                    enabled:            _hasLogs
                    Layout.fillWidth:   true
                    onClicked: {
                        rootLoader.sourceComponent = fetchLogsDialog
                        mainWindow.disableToolbar()
                    }
                }
            }
            Rectangle {
                width:  syncGrid.width
                height: 1
                color:  qgcPal.colorGrey
            }
            QGCButton {
                text:           qsTr("Disconnect")
                width:          _buttonWidth
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    TyphoonHQuickInterface.desktopSync.disconnectRemote()
                }
            }
        }
    }
    //-- Sync Files
    Component {
        id:             syncFilesDialog
        Rectangle {
            id:         syncFilesDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             syncFilesDialogShadow
                anchors.fill:   syncFilesDialogRect
                radius:         syncFilesDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       syncFilesDialogShadow
                visible:            syncFilesDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             syncFilesDialogShadow
            }
            Rectangle {
                id:     syncFilesDialogRect
                width:  syncCol.width  + (ScreenTools.defaultFontPixelWidth  * 8)
                height: syncCol.height + (ScreenTools.defaultFontPixelHeight * 8)
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 syncCol
                    width:              checksRect.width + (ScreenTools.defaultFontPixelWidth  * 8)
                    spacing:            ScreenTools.defaultFontPixelHeight * 2
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           _dialogTitle
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        id:             checksRect
                        color:          qgcPal.window
                        width:          syncFilesCol.width  + (ScreenTools.defaultFontPixelWidth * 4)
                        height:         syncFilesCol.height + ScreenTools.defaultFontPixelHeight
                        radius:         4
                        anchors.horizontalCenter: parent.horizontalCenter
                        Column {
                            id:         syncFilesCol
                            spacing:    ScreenTools.defaultFontPixelHeight
                            anchors.centerIn:   parent
                            ExclusiveGroup { id: syncTypeGroup }
                            QGCRadioButton {
                                checked:        TyphoonHQuickInterface.desktopSync.syncType === QGCSyncFilesDesktop.SyncClone
                                exclusiveGroup: syncTypeGroup
                                text:           qsTr("Clone (Existing files are replaced and extra files are removed)")
                                onClicked: {
                                    TyphoonHQuickInterface.desktopSync.syncType = QGCSyncFilesDesktop.SyncClone
                                }
                            }
                            QGCRadioButton {
                                checked:        TyphoonHQuickInterface.desktopSync.syncType === QGCSyncFilesDesktop.SyncReplace
                                exclusiveGroup: syncTypeGroup
                                text:           qsTr("Replace (Existing files are replaced)")
                                onClicked: {
                                    TyphoonHQuickInterface.desktopSync.syncType = QGCSyncFilesDesktop.SyncReplace
                                }
                            }
                            QGCRadioButton {
                                checked:        TyphoonHQuickInterface.desktopSync.syncType === QGCSyncFilesDesktop.SyncAppend
                                exclusiveGroup: syncTypeGroup
                                text:           qsTr("Append (Existing files are added with an unique name)")
                                onClicked: {
                                    TyphoonHQuickInterface.desktopSync.syncType = QGCSyncFilesDesktop.SyncAppend
                                }
                            }
                        }
                    }
                    ProgressBar {
                        width:          checksRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.syncProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.desktopSync.syncMessage
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:           !TyphoonHQuickInterface.desktopSync.sendingFiles ? kStart : kCancel
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            enabled:        !TyphoonHQuickInterface.desktopSync.syncDone && !TyphoonHQuickInterface.desktopSync.canceled
                            onClicked: {
                                if(TyphoonHQuickInterface.desktopSync.sendingFiles) {
                                    TyphoonHQuickInterface.desktopSync.cancelSync()
                                } else {
                                    if(_sendMission) {
                                        TyphoonHQuickInterface.desktopSync.uploadAllMissions()
                                    } else {
                                        TyphoonHQuickInterface.desktopSync.downloadAllMissions()
                                    }
                                }
                            }
                        }
                        QGCButton {
                            text:           kClose
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            enabled:        !TyphoonHQuickInterface.desktopSync.sendingFiles
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                TyphoonHQuickInterface.desktopSync.initSync()
                TyphoonHQuickInterface.desktopSync.syncMessage = _sendMission ? qsTr("Select missions to upload to remote") : qsTr("Select missions to download from remote")
                mainWindow.disableToolbar()
            }
        }
    }
    //-- Fetch Logs
    Component {
        id:             fetchLogsDialog
        Rectangle {
            id:         fetchLogsDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             fetchLogsDialogShadow
                anchors.fill:   fetchLogsDialogRect
                radius:         fetchLogsDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       fetchLogsDialogShadow
                visible:            fetchLogsDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             fetchLogsDialogShadow
            }
            Rectangle {
                id:                 fetchLogsDialogRect
                width:              ScreenTools.defaultFontPixelWidth * 100
                height:             fetchLogsCol.height * 1.25
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.alertBackground
                border.color:       qgcPal.alertBorder
                border.width:       2
                anchors.centerIn:   parent
                Column {
                    id:                 fetchLogsCol
                    width:              fetchLogsDialogRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 2
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Fetch Telemetry Logs From ") + TyphoonHQuickInterface.desktopSync.currentRemote
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        id:             logRect
                        color:          qgcPal.window
                        width:          logView.width  + (ScreenTools.defaultFontPixelWidth * 4)
                        height:         logView.height + ScreenTools.defaultFontPixelHeight
                        anchors.horizontalCenter: parent.horizontalCenter
                        ListView {
                            id:             logView
                            width:          (ScreenTools.defaultFontPixelWidth * (16 * 4)) + (ScreenTools.defaultFontPixelWidth * 8)
                            height:         qgcView.height * 0.35
                            spacing:        ScreenTools.defaultFontPixelWidth
                            orientation:    ListView.Vertical
                            model:          TyphoonHQuickInterface.desktopSync.logController.fileList
                            cacheBuffer:    Math.max(height * 2, 0)
                            clip:           true
                            highlightMoveDuration: 250
                            anchors.centerIn: parent
                            delegate: Row {
                                spacing: ScreenTools.defaultFontPixelWidth
                                anchors.horizontalCenter: parent.horizontalCenter
                                property var _fileItem: _hasLogs ? TyphoonHQuickInterface.desktopSync.logController.fileList[index] : null
                                QGCCheckBox {
                                    text:       ""
                                    checked:    _fileItem.selected
                                    onClicked:  _fileItem.selected = !_fileItem.selected
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QGCLabel {
                                    text:       _fileItem.fileName
                                    width:      logView.width * 0.55
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QGCLabel {
                                    text:       _fileItem.sizeStr
                                    width:      logView.width * 0.25
                                    horizontalAlignment: Text.AlignRight
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                    ProgressBar {
                        width:          logRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.syncProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    ProgressBar {
                        width:          logRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.fileProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.desktopSync.syncMessage
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:       kSelectAll
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  TyphoonHQuickInterface.desktopSync.logController.selectAllFiles(true)
                        }
                        QGCButton {
                            text:       kSelectNone
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  TyphoonHQuickInterface.desktopSync.logController.selectAllFiles(false)
                        }
                        QGCButton {
                            text:       !TyphoonHQuickInterface.desktopSync.sendingFiles ? kStart : kCancel
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            enabled:    TyphoonHQuickInterface.desktopSync.logController.selectedCount > 0 && !TyphoonHQuickInterface.desktopSync.canceled
                            onClicked: {
                                if(TyphoonHQuickInterface.desktopSync.sendingFiles) {
                                    TyphoonHQuickInterface.desktopSync.cancelSync()
                                } else {
                                    logDownloadFileDialog.open()
                                }
                            }
                            FolderDialog {
                                id:             logDownloadFileDialog
                                folder:         QGroundControl.settingsManager.appSettings.telemetrySavePath
                                onAccepted: {
                                    TyphoonHQuickInterface.desktopSync.downloadSelectedLogs(folder)
                                    close()
                                }
                            }
                        }
                        QGCButton {
                            text:       kClose
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                TyphoonHQuickInterface.desktopSync.initSync()
                TyphoonHQuickInterface.desktopSync.syncMessage = qsTr("Select telemetry logs to download from remote")
                mainWindow.disableToolbar()
            }
        }
    }
    //-- Fetch Maps
    Component {
        id:             fetchMapsDialog
        Rectangle {
            id:         fetchMapsDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             fetchMapsDialogShadow
                anchors.fill:   fetchMapsDialogRect
                radius:         fetchMapsDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       fetchMapsDialogShadow
                visible:            fetchMapsDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             fetchMapsDialogShadow
            }
            Rectangle {
                id:                 fetchMapsDialogRect
                width:              ScreenTools.defaultFontPixelWidth * 100
                height:             fetchLogsCol.height * 1.25
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.alertBackground
                border.color:       qgcPal.alertBorder
                border.width:       2
                anchors.centerIn:   parent
                Column {
                    id:                 fetchLogsCol
                    width:              fetchMapsDialogRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 2
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Fetch Map Tile Sets From ") + TyphoonHQuickInterface.desktopSync.currentRemote
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        id:             mapFetchRect
                        color:          qgcPal.window
                        width:          mapFetchViewCol.width  + (ScreenTools.defaultFontPixelWidth * 4)
                        height:         mapFetchViewCol.height + ScreenTools.defaultFontPixelHeight
                        anchors.horizontalCenter: parent.horizontalCenter
                        Column {
                            id:             mapFetchViewCol
                            spacing:        ScreenTools.defaultFontPixelHeight
                            ListView {
                                id:             mapFetchView
                                width:          (ScreenTools.defaultFontPixelWidth * (16 * 4)) + (ScreenTools.defaultFontPixelWidth * 8)
                                height:         qgcView.height * 0.35
                                spacing:        ScreenTools.defaultFontPixelWidth
                                orientation:    ListView.Vertical
                                model:          TyphoonHQuickInterface.desktopSync.mapController.fileList
                                cacheBuffer:    Math.max(height * 2, 0)
                                clip:           true
                                highlightMoveDuration: 250
                                anchors.centerIn: parent
                                delegate: Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    property var _fileItem: _hasMaps ? TyphoonHQuickInterface.desktopSync.mapController.fileList[index] : null
                                    QGCCheckBox {
                                        text:       ""
                                        checked:    _fileItem.selected
                                        onClicked:  _fileItem.selected = !_fileItem.selected
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCLabel {
                                        text:       _fileItem.fileName
                                        width:      mapFetchView.width * 0.55
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCLabel {
                                        text:       _fileItem.sizeStr
                                        width:      mapFetchView.width * 0.25
                                        horizontalAlignment: Text.AlignRight
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                            ExclusiveGroup { id: radioGroup }
                            Column {
                                spacing:            ScreenTools.defaultFontPixelHeight
                                width:              ScreenTools.defaultFontPixelWidth * 24
                                anchors.horizontalCenter: parent.horizontalCenter
                                QGCRadioButton {
                                    exclusiveGroup: radioGroup
                                    text:           qsTr("Append to existing set")
                                    checked:        !QGroundControl.mapEngineManager.importReplace
                                    onClicked:      QGroundControl.mapEngineManager.importReplace = !checked
                                    enabled:        QGroundControl.mapEngineManager.importAction === QGCMapEngineManager.ActionNone
                                }
                                QGCRadioButton {
                                    exclusiveGroup: radioGroup
                                    text:           qsTr("Replace existing set")
                                    checked:        QGroundControl.mapEngineManager.importReplace
                                    onClicked:      QGroundControl.mapEngineManager.importReplace = checked
                                    enabled:        QGroundControl.mapEngineManager.importAction === QGCMapEngineManager.ActionNone
                                }
                            }
                        }
                    }
                    ProgressBar {
                        width:          mapFetchRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.syncProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.desktopSync.syncMessage
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:       kSelectAll
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  TyphoonHQuickInterface.desktopSync.mapController.selectAllFiles(true)
                        }
                        QGCButton {
                            text:       kSelectNone
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  TyphoonHQuickInterface.desktopSync.mapController.selectAllFiles(false)
                        }
                        QGCButton {
                            text:       !TyphoonHQuickInterface.desktopSync.sendingFiles ? kStart : kCancel
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            enabled:    TyphoonHQuickInterface.desktopSync.mapController.selectedCount > 0 && !TyphoonHQuickInterface.desktopSync.canceled
                            onClicked: {
                                if(TyphoonHQuickInterface.desktopSync.sendingFiles) {
                                    TyphoonHQuickInterface.desktopSync.cancelSync()
                                } else {

                                }
                            }
                        }
                        QGCButton {
                            text:       kClose
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                TyphoonHQuickInterface.desktopSync.initSync()
                TyphoonHQuickInterface.desktopSync.syncMessage = qsTr("Select map tile sets to download from remote")
                mainWindow.disableToolbar()
            }
        }
    }
    //-- Upload Maps
    Component {
        id:             uploadMapsDlg
        Rectangle {
            id:         uploadMapsDlgItem
            width:      mainWindow.width
            height:     mainWindow.height
            color:      Qt.rgba(0,0,0,0.1)
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             uploadMapsDlgShadow
                anchors.fill:   uploadMapsDlgRect
                radius:         uploadMapsDlgRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       uploadMapsDlgShadow
                visible:            uploadMapsDlgRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             uploadMapsDlgShadow
            }
            Rectangle {
                id:                 uploadMapsDlgRect
                width:              ScreenTools.defaultFontPixelWidth * 100
                height:             uploadMapsCol.height * 1.25
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.alertBackground
                border.color:       qgcPal.alertBorder
                border.width:       2
                anchors.centerIn:   parent
                Column {
                    id:                 uploadMapsCol
                    width:              uploadMapsDlgRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 2
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Send Map Tiles To ") + TyphoonHQuickInterface.desktopSync.currentRemote
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Rectangle {
                        id:             logRect
                        color:          qgcPal.window
                        width:          tileSetList.width  + (ScreenTools.defaultFontPixelWidth * 4)
                        height:         tileSetList.height + ScreenTools.defaultFontPixelHeight
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCFlickable {
                            id:                 tileSetList
                            clip:               true
                            width:              (ScreenTools.defaultFontPixelWidth * (16 * 4)) + (ScreenTools.defaultFontPixelWidth * 8)
                            height:             qgcView.height * 0.35
                            contentHeight:      _cacheList.height
                            anchors.centerIn:   parent
                            Column {
                                id:             _cacheList
                                width:          tileSetList.width
                                spacing:        ScreenTools.defaultFontPixelHeight * 0.5
                                anchors.horizontalCenter: parent.horizontalCenter
                                Repeater {
                                    model: QGroundControl.mapEngineManager.tileSets
                                    delegate: Row {
                                        spacing: ScreenTools.defaultFontPixelWidth
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        QGCCheckBox {
                                            text:       ""
                                            checked:    object.selected
                                            onClicked:  object.selected = !object.selected
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        QGCLabel {
                                            text:       object.name
                                            width:      tileSetList.width * 0.55
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        QGCLabel {
                                            text:       object.totalTileCount
                                            width:      tileSetList.width * 0.25
                                            horizontalAlignment: Text.AlignRight
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }
                    }
                    ProgressBar {
                        width:          logRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.syncProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    ProgressBar {
                        width:          logRect.width
                        orientation:    Qt.Horizontal
                        minimumValue:   0
                        maximumValue:   100
                        value:          TyphoonHQuickInterface.desktopSync.fileProgress
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.desktopSync.syncMessage
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:       kSelectAll
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  QGroundControl.mapEngineManager.selectAll()
                        }
                        QGCButton {
                            text:       kSelectNone
                            width:      ScreenTools.defaultFontPixelWidth * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            onClicked:  QGroundControl.mapEngineManager.selectNone()
                        }
                        QGCButton {
                            text:       !TyphoonHQuickInterface.desktopSync.sendingFiles ? kStart : kCancel
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            enabled:    QGroundControl.mapEngineManager.selectedCount > 0 && !TyphoonHQuickInterface.desktopSync.canceled
                            onClicked: {
                                if(TyphoonHQuickInterface.desktopSync.sendingFiles) {
                                    TyphoonHQuickInterface.desktopSync.cancelSync()
                                } else {
                                    TyphoonHQuickInterface.desktopSync.uploadSelectedTiles()
                                }
                            }
                        }
                        QGCButton {
                            text:       kClose
                            width:      ScreenTools.defaultFontPixelWidth  * 16
                            enabled:    !TyphoonHQuickInterface.desktopSync.sendingFiles
                            height:     ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                QGroundControl.mapEngineManager.loadTileSets()
                TyphoonHQuickInterface.desktopSync.syncMessage = qsTr("Select tile set(s) to send to remote")
                mainWindow.disableToolbar()
            }
        }
    }
}
