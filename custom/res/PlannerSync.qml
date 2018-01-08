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
    property var    _syncType:          QGCRemote.SyncClone

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
                        _dialogTitle = qsTr("Send Missions")
                        dialogLoader.source = "/typhoonh/QGCSyncFilesDialog.qml"
                    }
                }
                QGCButton {
                    text:               qsTr("Fetch Missions")
                    width:              _gridElemWidth
                    Layout.fillWidth:   true
                    onClicked: {

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

    Loader {
        id:                 dialogLoader
        anchors.fill:       parent
    }
}
