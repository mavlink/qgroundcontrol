/*!
 * @file
 * @brief ST16 Settings Panel
 * @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtGraphicalEffects       1.0

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import TyphoonHQuickInterface               1.0
import TyphoonHQuickInterface.Widgets       1.0

QGCView {
    id:                 qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property real   _buttonWidth:     ScreenTools.defaultFontPixelWidth * 16
    property real   _textWidth:       ScreenTools.defaultFontPixelWidth * 40
    property bool   _importAction:    false

    QGCPalette      { id: qgcPal }

    function firmwareVersion() {
        if(_activeVehicle) {
            return qsTr("Firmware Version: " + _activeVehicle.firmwareCustomMajorVersion + "." + _activeVehicle.firmwareCustomMinorVersion + "." + _activeVehicle.firmwareCustomPatchVersion)
        } else {
            return ""
        }
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Column {
            id:                 settingsColumn
            width:              qgcView.width
            spacing:            ScreenTools.defaultFontPixelHeight
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.centerIn:   parent
            //-----------------------------------------------------------------
            Rectangle {
                height:         importRow.height * 2
                width:          ScreenTools.defaultFontPixelWidth * 80
                color:          qgcPal.windowShade
                anchors.horizontalCenter: parent.horizontalCenter
                Row {
                    id:         importRow
                    spacing:    ScreenTools.defaultFontPixelWidth * 4
                    anchors.centerIn: parent
                    QGCButton {
                        text:   qsTr("Import Mission")
                        width:   _buttonWidth
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            _importAction = true
                            rootLoader.sourceComponent = fileCopyDialog
                            mainWindow.disableToolbar()
                            TyphoonHQuickInterface.importMission()
                        }
                    }
                    QGCLabel {
                        text:   qsTr("Import missions from MicroSD Card.")
                        width:   _textWidth
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            //-----------------------------------------------------------------
            Rectangle {
                height:         exportRow.height * 2
                width:          ScreenTools.defaultFontPixelWidth * 80
                color:          qgcPal.windowShade
                anchors.horizontalCenter: parent.horizontalCenter
                Row {
                    id:         exportRow
                    spacing:    ScreenTools.defaultFontPixelWidth * 4
                    anchors.centerIn: parent
                    QGCButton {
                        text:   qsTr("Export Data")
                        width:   _buttonWidth
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            _importAction = false
                            rootLoader.sourceComponent = fileCopyDialog
                            mainWindow.disableToolbar()
                            TyphoonHQuickInterface.exportData()
                        }
                    }
                    QGCLabel {
                        text:   qsTr("Export missions and logs to MicroSD Card.")
                        width:   _textWidth
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            //-----------------------------------------------------------------
            Rectangle {
                height:         bindRow.height * 2
                width:          ScreenTools.defaultFontPixelWidth * 80
                color:          qgcPal.windowShade
                visible:        !_activeVehicle || (_activeVehicle.rcRSSI === 0 || _activeVehicle.rcRSSI === 255)
                anchors.horizontalCenter: parent.horizontalCenter
                Row {
                    id:         bindRow
                    spacing:    ScreenTools.defaultFontPixelWidth * 4
                    anchors.centerIn: parent
                    QGCButton {
                        text:   qsTr("Manual Bind")
                        width:   _buttonWidth
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            rootLoader.sourceComponent = bindDialog
                            mainWindow.disableToolbar()
                        }
                    }
                    QGCLabel {
                        text:   qsTr("Manually bind RC to vehicle.")
                        width:   _textWidth
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
            QGCLabel {
                visible: _activeVehicle
                anchors.horizontalCenter: parent.horizontalCenter
                text: firmwareVersion()
            }
        }
    }
    //-- Import Files
    Component {
        id:             fileCopyDialog
        Item {
            id:         fileCopyDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            z:          1000000
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             fileCopyDialogShadow
                anchors.fill:   fileCopyDialogRect
                radius:         fileCopyDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       fileCopyDialogShadow
                visible:            fileCopyDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             fileCopyDialogShadow
            }
            Rectangle {
                id:     fileCopyDialogRect
                width:  mainWindow.width   * 0.65
                height: copyCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 copyCol
                    width:              fileCopyDialogRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           _importAction ? qsTr("Import Mission Files") : qsTr("Export Data Files")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text: {
                            if(_importAction) {
                                return qsTr("Importing ") + TyphoonHQuickInterface.copyResult.toString() + qsTr(" files")
                            } else {
                                return qsTr("Exporting ") + TyphoonHQuickInterface.copyResult.toString() + qsTr(" files")
                            }
                        }
                        color:          qgcPal.alertText
                        visible:        TyphoonHQuickInterface.copyingFiles
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           TyphoonHQuickInterface.copyResult.toString() + (_importAction ? qsTr(" files imported") : qsTr(" files exported"))
                        color:          qgcPal.alertText
                        visible:        !TyphoonHQuickInterface.copyingFiles && TyphoonHQuickInterface.copyResult >= 0
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("Error copying files");
                        color:          qgcPal.alertText
                        visible:        !TyphoonHQuickInterface.copyingFiles && TyphoonHQuickInterface.copyResult < 0
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCButton {
                        text:           qsTr("Close")
                        width:          ScreenTools.defaultFontPixelWidth  * 16
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        visible:        !TyphoonHQuickInterface.copyingFiles
                        anchors.horizontalCenter: parent.horizontalCenter
                        onClicked: {
                            rootLoader.sourceComponent = null
                            mainWindow.enableToolbar()
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = fileCopyDialogItem.width
                rootLoader.height = fileCopyDialogItem.height
            }
        }
    }
    //-- Manual Bind Dialog
    Component {
        id:             bindDialog
        Item {
            id:         bindDialogItem
            width:      mainWindow.width
            height:     mainWindow.height
            z:          1000000
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            Rectangle {
                id:             bindDialogShadow
                anchors.fill:   bindDialogRect
                radius:         bindDialogRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       bindDialogShadow
                visible:            bindDialogRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             bindDialogShadow
            }
            Rectangle {
                id:     bindDialogRect
                width:  mainWindow.width   * 0.65
                height: bindCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 bindCol
                    width:              bindDialogRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("Manual RC Bind")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("Flip vehicle upside down and hit the Bind button")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 4
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCButton {
                            text:           qsTr("Bind")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
                            height:         ScreenTools.defaultFontPixelHeight * 2
                            onClicked: {
                                TyphoonHQuickInterface.manualBind()
                                rootLoader.sourceComponent = null
                                mainWindow.enableToolbar()
                            }
                        }
                        QGCButton {
                            text:           qsTr("Cancel")
                            width:          ScreenTools.defaultFontPixelWidth  * 16
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
                rootLoader.width  = bindDialogItem.width
                rootLoader.height = bindDialogItem.height
            }
        }
    }
}
