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
import QtGraphicalEffects   1.0

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
    width:  _indicatorDiameter
    radius: ScreenTools.defaultFontPixelWidth * 0.5
    color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string _commLostStr: qsTr("NO CAMERA")

    property real _spacers:                 ScreenTools.defaultFontPixelHeight * 0.5
    property real _editFieldWidth:          ScreenTools.defaultFontPixelWidth  * 30
    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:       _activeVehicle ? _activeVehicle.connectionLost : false
    property bool _emptySD:                 TyphoonHQuickInterface.cameraControl.sdTotal === 0
    property bool _cameraVideoMode:         !_communicationLost && (_emptySD ? false : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_VIDEO)
    property bool _cameraPhotoMode:         !_communicationLost && (_emptySD ? false : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_PHOTO)
    property bool _cameraModeUndefined:     _communicationLost  || (_emptySD ? true  : TyphoonHQuickInterface.cameraControl.cameraMode  === CameraControl.CAMERA_MODE_UNDEFINED)
    property bool _cameraAutoMode:          TyphoonHQuickInterface.cameraControl ? TyphoonHQuickInterface.cameraControl.aeMode === CameraControl.AE_MODE_AUTO : false;

    property real _mediaWidth:              128
    property real _mediaHeight:             72
    property real _mediaIndex:              0
    property var  _mediaModel:              []

    function baseName(str) {
        return (str.slice(str.lastIndexOf("/")+1))
    }

    Connections {
        target: TyphoonHQuickInterface.cameraControl
        onCameraAvailableChanged: {
            if(!TyphoonHQuickInterface.cameraControl.cameraAvailable) {
                rootLoader.sourceComponent = noCameraDlg
            }
        }
    }

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
            text:       _activeVehicle ? (TyphoonHQuickInterface.cameraControl.cameraModel !== "" ? TyphoonHQuickInterface.cameraControl.cameraModel : _commLostStr) : _commLostStr
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.horizontalCenter: parent.horizontalCenter
        }
        //-- Camera Mode
        Item {
            width:  ScreenTools.defaultFontPixelHeight * 4
            height: ScreenTools.defaultFontPixelHeight * 4
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: _cameraModeUndefined ? 0.5 : 1
            QGCColoredImage {
                anchors.fill:       parent
                source:             (_cameraModeUndefined || _cameraVideoMode) ? "/typhoonh/img/camera_switch_video.svg" : "/typhoonh/img/camera_switch_photo.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
                QGCColoredImage {
                    anchors.fill:       parent
                    source:             (_cameraModeUndefined || _cameraVideoMode) ? "/typhoonh/img/camera_switch_video_mode.svg" : "/typhoonh/img/camera_switch_photo_mode.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _cameraModeUndefined ? qgcPal.text : qgcPal.colorGreen
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
            width:      parent.width * 0.5
            height:     width
            radius:     width * 0.5
            border.color: qgcPal.buttonText
            border.width: 3
            anchors.horizontalCenter: parent.horizontalCenter
            Rectangle {
                width:      parent.width * 0.75
                height:     width
                radius:     width * 0.5
                color:      _cameraModeUndefined ? qgcPal.colorGrey : ( _cameraVideoMode ? qgcPal.colorRed : "white" )
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
            visible: _cameraVideoMode
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
            visible: !_cameraVideoMode
        }
        Rectangle {
            height: 1
            width:  parent.width * 0.85
            color:  qgcPal.globalTheme === QGCPalette.Dark ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.25)
            anchors.horizontalCenter: parent.horizontalCenter
        }
        //-- Media Player
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
        }
        QGCColoredImage {
            width:              ScreenTools.defaultFontPixelHeight * 1.5
            height:             width
            sourceSize.width:   width
            source:             "qrc:/typhoonh/img/mediaPlay.svg"
            fillMode:           Image.PreserveAspectFit
            color:              (!_communicationLost && TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED) ? qgcPal.text : qgcPal.colorGrey
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    if(rootLoader.sourceComponent === null) {
                        rootLoader.sourceComponent = mediaPlayerComponent
                    } else {
                        rootLoader.sourceComponent = null
                    }
                }
            }
        }
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
        }
        //-- Settings
        QGCColoredImage {
            width:              ScreenTools.defaultFontPixelHeight * 2.5
            height:             width
            sourceSize.width:   width
            source:             "qrc:/typhoonh/img/sliders.svg"
            fillMode:           Image.PreserveAspectFit
            color:              (!_communicationLost && TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED) ? qgcPal.text : qgcPal.colorGrey
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                enabled:        !_communicationLost && TyphoonHQuickInterface.cameraControl.cameraMode !== CameraControl.CAMERA_MODE_UNDEFINED
                onClicked: {
                    if(rootLoader.sourceComponent === null) {
                        rootLoader.sourceComponent = cameraSettingsComponent
                    } else {
                        rootLoader.sourceComponent = null
                    }
                }
            }
        }
        Item {
            height:     ScreenTools.defaultFontPixelHeight
            width:      1
        }
    }

    Component {
        id: cameraSettingsComponent
        Item {
            id:     cameraSettingsRect
            z:      100000
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    rootLoader.sourceComponent = null
                }
            }
            Rectangle {
                id:             ccamSettingsShadow
                anchors.fill:   camSettingsRect
                radius:         camSettingsRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       ccamSettingsShadow
                visible:            camSettingsRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             ccamSettingsShadow
            }
            Rectangle {
                id:     camSettingsRect
                width:  mainWindow.width  * 0.45
                height: mainWindow.height * 0.65
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:   1
                border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn: parent
                QGCLabel {
                    id:                 cameraSettingsLabel
                    text:               _cameraVideoMode ? "Video Settings" : "Camera Settings"
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
                                enabled:     _cameraVideoMode && TyphoonHQuickInterface.cameraControl.videoStatus !== CameraControl.VIDEO_CAPTURE_STATUS_RUNNING
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
                                checked:     _cameraAutoMode
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
                                enabled:    _cameraAutoMode
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
                                enabled:    !_cameraAutoMode
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
                                enabled:    !_cameraAutoMode
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
                                enabled:     _cameraPhotoMode
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
                                enabled:     _cameraPhotoMode
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
                                enabled:     _cameraAutoMode
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
                                enabled:     !_emptySD
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
    //-- Connected to some AP and not a Typhoon
    Component {
        id:             noCameraDlg
        Item {
            id:         noCameraItem
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
                id:             noCameraShadow
                anchors.fill:   noCameraRect
                radius:         noCameraRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       noCameraShadow
                visible:            noCameraRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             noCameraShadow
            }
            Rectangle {
                id:     noCameraRect
                width:  mainWindow.width   * 0.65
                height: noCameraCol.height * 1.5
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.alertBackground
                border.color: qgcPal.alertBorder
                border.width: 2
                anchors.centerIn: parent
                Column {
                    id:                 noCameraCol
                    width:              noCameraRect.width
                    spacing:            ScreenTools.defaultFontPixelHeight * 3
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.centerIn:   parent
                    QGCLabel {
                        text:           qsTr("No Camera Reported")
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.largeFontPointSize
                        color:          qgcPal.alertText
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           qsTr("Vehicle did not report camera available.")
                        color:          qgcPal.alertText
                        font.family:    ScreenTools.demiboldFontFamily
                        font.pointSize: ScreenTools.mediumFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCButton {
                        text:           qsTr("Close")
                        width:          ScreenTools.defaultFontPixelWidth  * 10
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        onClicked:      rootLoader.sourceComponent = null
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = noCameraItem.width
                rootLoader.height = noCameraItem.height
            }
        }
    }
    //-- Media Player
    Component {
        id: mediaPlayerComponent
        Item {
            id:     mediaPlayerItem
            z:      100000
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    rootLoader.sourceComponent = null
                }
            }
            Rectangle {
                id:             mediaPlayerShadow
                anchors.fill:   mediaPlayerRect
                radius:         mediaPlayerRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       mediaPlayerShadow
                visible:            mediaPlayerRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             mediaPlayerShadow
            }
            Rectangle {
                id:                 mediaPlayerRect
                width:              mediaPlayerGrid.width  + 60
                height:             mediaPlayerGrid.height + 60
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:       1
                border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn:   parent
                MouseArea {
                    anchors.fill:   parent
                    onWheel:        { wheel.accepted = true; }
                    onPressed:      { mouse.accepted = true; }
                    onReleased:     { mouse.accepted = true; }
                }
                GridView {
                    id:                 mediaPlayerGrid
                    clip:               true
                    model:              _mediaModel
                    cellWidth:          _mediaWidth  + 8
                    cellHeight:         _mediaHeight + 8
                    width:              (_mediaWidth  + 8) * 4
                    height:             (_mediaHeight + 8) * 4
                    anchors.centerIn:   parent
                    delegate:           mediaDelegate
                    cacheBuffer:        height * 4
                }
                QGCLabel {
                    text:       qsTr("No Images")
                    visible:    _mediaModel.length < 1
                    anchors.centerIn: parent
                }
            }
            Component.onCompleted: {
                rootLoader.width  = mediaPlayerItem.width
                rootLoader.height = mediaPlayerItem.height
                _mediaModel = TyphoonHQuickInterface.getMediaList()
            }
            Keys.onBackPressed: {
                rootLoader.sourceComponent = null
            }
        }
    }
    Component {
        id: mediaDelegate
        Image {
            width:      _mediaWidth
            height:     _mediaHeight
            fillMode:   Image.PreserveAspectFit
            source:     "file://" + modelData
            sourceSize.width: width
            //asynchronous:     true
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    _mediaIndex = index
                    rootLoader.sourceComponent = mediaViewComponent
                }
            }
        }
    }
    //-- Media Viewer
    Component {
        id: mediaViewComponent
        Item {
            id:     mediaViewItem
            z:      100000
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    rootLoader.sourceComponent = mediaPlayerComponent
                }
            }
            Rectangle {
                id:             mediaViewShadow
                anchors.fill:   mediaViewRect
                radius:         mediaViewRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                anchors.fill:       mediaViewShadow
                visible:            mediaViewRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             mediaViewShadow
            }
            Rectangle {
                id:                 mediaViewRect
                width:              mediaContent.width   + 120
                height:             mediaContent.height  + 80
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:       1
                border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn:   parent
                MouseArea {
                    anchors.fill:   parent
                    onWheel:        { wheel.accepted = true; }
                    onPressed:      { mouse.accepted = true; }
                    onReleased:     { mouse.accepted = true; }
                }
                Image {
                    id:             mediaContent
                    width:          _mediaWidth  * 6
                    height:         _mediaHeight * 6
                    fillMode:       Image.PreserveAspectFit
                    source:         "file://" + _mediaModel[_mediaIndex]
                    anchors.centerIn:   parent
                }
                QGCLabel {
                    text:           baseName(mediaContent.source.toString())
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                //-- Previous Image
                QGCColoredImage {
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left:       parent.left
                    width:              ScreenTools.defaultFontPixelHeight * 2
                    height:             width
                    sourceSize.height:  width
                    source:             "qrc:/typhoonh/img/mediaBack.svg"
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    visible:            _mediaIndex > 0
                    color:              qgcPal.text
                    MouseArea {
                        anchors.fill:   parent
                        onClicked: {
                            if(_mediaIndex > 0) {
                                _mediaIndex = _mediaIndex - 1;
                                mediaContent.source = "file://" + _mediaModel[_mediaIndex]
                            }
                        }
                    }
                }
                //-- Next Image
                QGCColoredImage {
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right:      parent.right
                    width:              ScreenTools.defaultFontPixelHeight * 2
                    height:             width
                    sourceSize.height:  width
                    source:             "qrc:/typhoonh/img/mediaPlay.svg"
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    visible:            _mediaIndex < (_mediaModel.length - 1)
                    color:              qgcPal.text
                    MouseArea {
                        anchors.fill:   parent
                        onClicked: {
                            if(_mediaIndex < (_mediaModel.length - 1)) {
                                _mediaIndex = _mediaIndex + 1;
                                mediaContent.source = "file://" + _mediaModel[_mediaIndex]
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = mediaViewItem.width
                rootLoader.height = mediaViewItem.height
            }
            Keys.onBackPressed: {
                rootLoader.sourceComponent = null
            }
        }
    }
}

