/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.4
import QtQuick.Dialogs      1.3
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.QGCRemote             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

import TyphoonHQuickInterface               1.0

Item {
    id:         syncFilesDialogItem
    width:      mainWindow.width
    height:     mainWindow.height

    property var    _syncType:          QGCRemote.SyncClone

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
                        checked:        TyphoonHQuickInterface.desktopSync.qgcRemote && TyphoonHQuickInterface.desktopSync.qgcRemote.syncType === QGCRemote.SyncClone
                        exclusiveGroup: syncTypeGroup
                        text:           "Clone"
                        onClicked: {
                        }
                    }
                    QGCRadioButton {
                        checked:        TyphoonHQuickInterface.desktopSync.qgcRemote && TyphoonHQuickInterface.desktopSync.qgcRemote.syncType === QGCRemote.SyncReplace
                        exclusiveGroup: syncTypeGroup
                        text:           "Replace"
                        onClicked: {
                        }
                    }
                    QGCRadioButton {
                        checked:        TyphoonHQuickInterface.desktopSync.qgcRemote && TyphoonHQuickInterface.desktopSync.qgcRemote.syncType === QGCRemote.SyncAppend
                        exclusiveGroup: syncTypeGroup
                        text:           "Append"
                        onClicked: {
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
                            TyphoonHQuickInterface.desktopSync.uploadAllMissions()
                        }
                    }
                }
                QGCButton {
                    text:           qsTr("Close")
                    width:          ScreenTools.defaultFontPixelWidth  * 16
                    enabled:        !TyphoonHQuickInterface.desktopSync.sendingFiles
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    onClicked: {
                        dialogLoader.source = ""
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
