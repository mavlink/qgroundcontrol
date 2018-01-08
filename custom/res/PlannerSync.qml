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

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
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
    property real   _buttonWidth:       ScreenTools.defaultFontPixelWidth * 16
    property real   _gridElemWidth:     ScreenTools.defaultFontPixelWidth * 20
    property string _currentNode:       _clientCount && remoteCombo.currentIndex >= 0 && remoteCombo.currentIndex < _clientCount ? TyphoonHQuickInterface.desktopSync.remoteList[remoteCombo.currentIndex] : ""
    property string _dialogTitle:       ""
    property bool   _sendMission:       true

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
        QGCLabel {
            text:           qsTr("No remotes detected")
            font.family:    ScreenTools.demiboldFontFamily
            font.pointSize: ScreenTools.mediumFontPointSize
            visible:        !_activeVehicle && !_clientCount
            anchors.centerIn: parent
        }
        Column {
            spacing:        ScreenTools.defaultFontPixelHeight
            visible:        !_activeVehicle && _clientCount && !TyphoonHQuickInterface.desktopSync.remoteReady
            anchors.centerIn: parent
            QGCLabel {
                text:       qsTr("Select Remote")
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
        Column {
            anchors.centerIn: parent
            spacing:        ScreenTools.defaultFontPixelHeight
            visible:        !_activeVehicle && TyphoonHQuickInterface.desktopSync.remoteReady
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
                    text:           qsTr("Send Missions")
                    width:          _gridElemWidth
                    onClicked: {
                        _sendMission = true
                        _dialogTitle = qsTr("Send Missions")
                        rootLoader.sourceComponent = syncFilesDialog
                        mainWindow.disableToolbar()
                    }
                }
                QGCButton {
                    text:               qsTr("Fetch Missions")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {
                        _sendMission = false
                        _dialogTitle = qsTr("Fetch Missions")
                        rootLoader.sourceComponent = syncFilesDialog
                        mainWindow.disableToolbar()
                    }
                }
                QGCButton {
                    text:               qsTr("Send Maps")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {

                    }
                }
                QGCButton {
                    text:               qsTr("Fetch Maps")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {

                    }
                }
                Item {
                    height:             1
                    width:              1
                }
                QGCButton {
                    text:               qsTr("Fetch Logs")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {

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
    //-- Sync Files (Outgoing)
    Component {
        id:             syncFilesDialog
        Item {
            id:         syncFilesDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
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
                width:  mainWindow.width   * 0.65
                height: syncCol.height * 1.25
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 syncCol
                    width:              syncFilesDialogRect.width
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
                        width:          parent.width * 0.75
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
                            text:           !TyphoonHQuickInterface.desktopSync.sendingFiles ? qsTr("Start") : qsTr("Cancel")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            enabled:        !TyphoonHQuickInterface.desktopSync.syncDone
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
                            text:           qsTr("Close")
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
                rootLoader.width  = syncFilesDialogItem.width
                rootLoader.height = syncFilesDialogItem.height
                mainWindow.disableToolbar()
            }
        }
    }
}
