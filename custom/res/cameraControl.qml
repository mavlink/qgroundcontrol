/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief Fly View Instrument Widget
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick              2.4
import QtPositioning        5.2
import QtQuick.Layouts      1.2
import QtQuick.Dialogs      1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Palette       1.0
import QGroundControl.CameraControl 1.0

import TyphoonHQuickInterface           1.0
import TyphoonHQuickInterface.Widgets   1.0

Rectangle {
    id:     mainRect
    height: mainCol.height
    width:  ScreenTools.defaultFontPixelWidth * 12
    radius: ScreenTools.defaultFontPixelWidth
    color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0.85,0.85,1,0.85) : Qt.rgba(0.15,0.15,0.25,0.85)
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? "white" : "black"

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property real _spacers:                 ScreenTools.defaultFontPixelHeight * 0.5
    property real _editFieldWidth:          ScreenTools.defaultFontPixelWidth  * 30
    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:       _activeVehicle ? _activeVehicle.connectionLost : false
    property bool _cameraVideoMode:         !_communicationLost && (TyphoonHQuickInterface.cameraControl.sdTotal === 0 ? false : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_VIDEO)
    property bool _cameraPhotoMode:         !_communicationLost && (TyphoonHQuickInterface.cameraControl.sdTotal === 0 ? false : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_PHOTO)
    property bool _cameraModeUndefined:     _communicationLost  || (TyphoonHQuickInterface.cameraControl.sdTotal === 0 ? true  : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_UNDEFINED)

    /*
    Connections {
        target: TyphoonHQuickInterface.cameraControl
        onCameraModeChanged: {
            console.log('QML: Camera Mode Changed: ' + TyphoonHQuickInterface.cameraControl.cameraMode)
        }
        onRecordTimeChanged: {
            console.log('QML: Record Time: ' + TyphoonHQuickInterface.cameraControl.recordTime)
        }
        onVideoStatusChanged: {
            console.log('QML: Video Status: ' + TyphoonHQuickInterface.cameraControl.videoStatus + ' ' + CameraControl.VIDEO_CAPTURE_STATUS_STOPPED)
        }
    }
    */

    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }

    Column {
        id:         mainCol
        width:      parent.width
        spacing:    _spacers
        anchors.centerIn: parent
        Item {
            height:     _spacers
            width:      1
        }
        //-----------------------------------------------------------------
        QGCLabel {
            id:         cameraLabel
            text:       TyphoonHQuickInterface.connectedCamera === "" ? qsTr("NO CAMERA") : TyphoonHQuickInterface.connectedCamera
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.horizontalCenter: parent.horizontalCenter
        }
        //-- Camera Mode
        Rectangle {
            width:  cameraModeRow.width
            height: ScreenTools.defaultFontPixelHeight * 2
            radius: width * 0.5
            color:  "black"
            anchors.horizontalCenter: parent.horizontalCenter
            Row {
                id:     cameraModeRow
                spacing: ScreenTools.defaultFontPixelWidth * 1.5
                //-- Camera
                Rectangle {
                    height:                 ScreenTools.defaultFontPixelHeight * 1.95
                    width:                  height
                    radius:                 width * 0.5
                    color:                  _cameraPhotoMode ? qgcPal.colorGreen : "black"
                    QGCColoredImage {
                        height:             parent.height * 0.65
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/img/camera.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              _cameraPhotoMode ? "black" : qgcPal.colorGrey
                        anchors.centerIn:   parent
                    }
                }
                //-- Video
                Rectangle {
                    height:                 ScreenTools.defaultFontPixelHeight * 1.95
                    width:                  height
                    radius:                 width * 0.5
                    color:                  _cameraVideoMode ? qgcPal.colorGreen : "black"
                    QGCColoredImage {
                        height:             parent.height * 0.65
                        width:              height
                        sourceSize.width:   width
                        source:             "qrc:/typhoonh/img/video.svg"
                        fillMode:           Image.PreserveAspectFit
                        color:              _cameraVideoMode ? "black" : qgcPal.colorGrey
                        anchors.centerIn:   parent
                    }
                }
            }
            MouseArea {
                anchors.fill:   parent
                enabled:        !_cameraModeUndefined
                onClicked: {
                    rootLoader.sourceComponent = null
                    TyphoonHQuickInterface.cameraControl.toggleMode()
                }
            }
        }
        //-- Shutter
        Rectangle {
            color:      Qt.rgba(0,0,0,0)
            width:      parent.width * 0.75
            height:     width
            radius:     width * 0.5
            border.color: qgcPal.buttonText
            border.width: 3
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width:      parent.width * 0.75
                height:     width
                radius:     width * 0.5
                color:      _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.colorRed
                visible:    !pauseVideo.visible
                anchors.centerIn:   parent
            }
            Rectangle {
                id:         pauseVideo
                width:      parent.width * 0.5
                height:     width
                color:      _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.colorRed
                visible:    _cameraVideoMode && TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING
                anchors.centerIn:   parent
            }
            MouseArea {
                anchors.fill:   parent
                enabled:        !_cameraModeUndefined
                onClicked: {
                    rootLoader.sourceComponent = null
                    if(_cameraVideoMode) {
                        if(TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING) {
                            TyphoonHQuickInterface.cameraControl.stopVideo()
                        } else {
                            TyphoonHQuickInterface.cameraControl.startVideo()
                        }
                    } else {
                        TyphoonHQuickInterface.cameraControl.takePhoto()
                    }
                }
            }
        }
        //-- Recording Time
        QGCLabel {
            text: (_cameraVideoMode && TyphoonHQuickInterface.cameraControl.videoStatus === CameraControl.VIDEO_CAPTURE_STATUS_RUNNING) ? TyphoonHQuickInterface.cameraControl.recordTimeStr : "00:00:00"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        QGCColoredImage {
            width:              parent.width * 0.45
            height:             width
            sourceSize.width:   width
            source:             "qrc:/typhoonh/img/sliders.svg"
            fillMode:           Image.PreserveAspectFit
            color:              _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.text
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                enabled:        !_cameraModeUndefined
                onClicked: {
                    console.log('mainWindow ' + mainWindow.width + ' ' + mainWindow.height)
                    if(rootLoader.sourceComponent === null) {
                        rootLoader.sourceComponent = cameraSettingsComponent
                    } else {
                        rootLoader.sourceComponent = null
                    }
                }
            }
        }
        Item {
            height:     _spacers
            width:      1
        }
    }

    Component {
        id: cameraSettingsComponent
        Rectangle {
            id:     cameraSettingsRect
            width:  mainWindow.width  * 0.45
            height: mainWindow.height * 0.675
            radius: ScreenTools.defaultFontPixelWidth
            color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.75) : Qt.rgba(0,0,0,0.75)
            anchors.centerIn: parent
            MouseArea {
                anchors.fill:   parent
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            QGCLabel {
                id:                 cameraSettingsLabel
                text:               "Camera Settings"
                font.family:        ScreenTools.demiboldFontFamily
                font.pointSize:     ScreenTools.mediumFontPointSize
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.left:       parent.left
            }
            QGCFlickable {
                clip:               true
                anchors.top:        cameraSettingsLabel.bottom
                anchors.bottom:     parent.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                contentHeight:      cameraSettingsCol.height
                contentWidth:       cameraSettingsCol.width
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                Column {
                    id:                 cameraSettingsCol
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.15
                    width:              cameraGrid.width
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    GridLayout {
                        id:             cameraGrid
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     columnSpacing * 0.5
                        columns:        2
                        anchors.horizontalCenter: parent.horizontalCenter
                        //-------------------------------------------
                        //-- Video Recording Resolution
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Video Resolution"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.videoResList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentVideoRes
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentVideoRes = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                        }
                        //-------------------------------------------
                        //-- White Balance
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "White Balance"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.wbList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentWB
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentWB = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                        }
                        //-------------------------------------------
                        //-- AE Mode
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Auto Exposure"
                            Layout.fillWidth: true
                        }
                        OnOffSwitch {
                            checked:     TyphoonHQuickInterface.cameraControl.aeMode === CameraControl.AE_MODE_AUTO
                            Layout.alignment: Qt.AlignRight
                            onClicked:  TyphoonHQuickInterface.cameraControl.aeMode = checked ? CameraControl.AE_MODE_AUTO : CameraControl.AE_MODE_MANUAL
                        }
                        //-------------------------------------------
                        //-- EV (auto)
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "EV Compensation"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.evList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentEV
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentEV = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                            enabled:    TyphoonHQuickInterface.cameraControl.aeMode === CameraControl.AE_MODE_AUTO
                        }
                        //-------------------------------------------
                        //-- ISO (manual)
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "ISO"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.isoList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentIso
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentIso = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                            enabled:    TyphoonHQuickInterface.cameraControl.aeMode !== CameraControl.AE_MODE_AUTO
                        }
                        //-------------------------------------------
                        //-- Shutter Speed (manual)
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Shutter Speed"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.shutterList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentShutter
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentShutter = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                            enabled:    TyphoonHQuickInterface.cameraControl.aeMode !== CameraControl.AE_MODE_AUTO
                        }
                        //-------------------------------------------
                        //-- Color "IQ" Mode
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Color Mode"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.iqModeList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentIQ
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentIQ = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                        }
                        //-------------------------------------------
                        //-- Photo File Format
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Photo Format"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.photoFormatList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentPhotoFmt
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentPhotoFmt = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                        }
                        //-------------------------------------------
                        //-- Metering Mode
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Metering Mode"
                            Layout.fillWidth: true
                        }
                        QGCComboBox {
                            width:       _editFieldWidth
                            model:       TyphoonHQuickInterface.cameraControl.meteringList
                            currentIndex:TyphoonHQuickInterface.cameraControl.currentMetering
                            onActivated: {
                                TyphoonHQuickInterface.cameraControl.currentMetering = index
                            }
                            Layout.preferredWidth:  _editFieldWidth
                        }
                        //-------------------------------------------
                        //-- Screen Grid
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Screen Grid"
                            Layout.fillWidth: true
                        }
                        OnOffSwitch {
                            checked:     QGroundControl.settingsManager.videoSettings.gridLines.rawValue
                            Layout.alignment: Qt.AlignRight
                            onClicked:  QGroundControl.settingsManager.videoSettings.gridLines.rawValue = checked
                        }
                        //-------------------------------------------
                        //-- Reset Camera
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Reset Camera Defaults"
                            Layout.fillWidth: true
                        }
                        QGCButton {
                            text:       "Reset"
                            onClicked:  resetPrompt.open()
                            Layout.preferredWidth:  _editFieldWidth
                            MessageDialog {
                                id:                 resetPrompt
                                title:              qsTr("Reset Camera to Factory Settings")
                                text:               qsTr("Confirm resetting all settings?")
                                standardButtons:    StandardButton.Yes | StandardButton.No
                                onNo: resetPrompt.close()
                                onYes: {
                                    TyphoonHQuickInterface.cameraControl.resetSettings()
                                    resetPrompt.close()
                                }
                            }
                        }
                        //-------------------------------------------
                        //-- Format SD Card
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                        QGCLabel {
                            text:       "Micro SD Card"
                            Layout.fillWidth: true
                        }
                        QGCButton {
                            text:       "Format"
                            onClicked:  formatPrompt.open()
                            Layout.preferredWidth:  _editFieldWidth
                            MessageDialog {
                                id:                 formatPrompt
                                title:              qsTr("Format MicroSD Card")
                                text:               qsTr("Confirm erasing all files?")
                                standardButtons:    StandardButton.Yes | StandardButton.No
                                onNo: formatPrompt.close()
                                onYes: {
                                    TyphoonHQuickInterface.cameraControl.formatCard()
                                    formatPrompt.close()
                                }
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      mainWindow.width * 0.4
                            Layout.columnSpan: 2
                            Layout.maximumHeight: 2
                        }
                    }
                }
            }

            //-- Dismiss Window
            Image {
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.top:        parent.top
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelHeight * 1.5
                height:             width
                sourceSize.height:  width
                source:             "/res/XDelete.svg"
                fillMode:           Image.PreserveAspectFit
                mipmap:             true
                smooth:             true
                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        rootLoader.sourceComponent = null
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = cameraSettingsRect.width
                rootLoader.height = cameraSettingsRect.height
            }
            Keys.onBackPressed: {
                rootLoader.sourceComponent = null
            }
        }
    }
}
