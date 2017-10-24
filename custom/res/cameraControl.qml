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

import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Vehicle           1.0

import TyphoonHQuickInterface           1.0
import TyphoonHQuickInterface.Widgets   1.0
import TyphoonMediaItem                 1.0

Rectangle {
    id:         mainRect
    height:     mainCol.height
    width:      _isThermal ? _indicatorDiameter * 0.75 : _indicatorDiameter
    visible:    !QGroundControl.videoManager.fullScreen
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    border.width:   1
    border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string _commLostStr: qsTr("NO CAMERA")

    property real _spacers:                 ScreenTools.defaultFontPixelHeight * 0.35
    property real _labelFieldWidth:         ScreenTools.defaultFontPixelWidth * 30
    property real _editFieldWidth:          ScreenTools.defaultFontPixelWidth * 30

    property var  _activeVehicle:           QGroundControl.multiVehicleManager.activeVehicle
    property var  _dynamicCameras:          _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool _isCamera:                _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var  _camera:                  _isCamera ? _dynamicCameras.cameras.get(0) : null // Single camera support for the time being
    property bool _communicationLost:       _activeVehicle ? _activeVehicle.connectionLost : false
    property bool _emptySD:                 _camera && _camera.storageTotal === 0
    property bool _cameraVideoMode:         !_communicationLost && (_emptySD ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_VIDEO)
    property bool _cameraPhotoMode:         !_communicationLost && (_emptySD ? false : _camera && (_camera.cameraMode  === QGCCameraControl.CAM_MODE_PHOTO || _camera.cameraMode === QGCCameraControl.CAM_MODE_SURVEY))
    property bool _cameraPhotoIdle:         !_communicationLost && (_emptySD ? false : _camera && _camera.photoStatus  === QGCCameraControl.PHOTO_CAPTURE_IDLE)
    property bool _cameraElapsedMode:       !_communicationLost && (_emptySD ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_PHOTO && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE)
    property bool _cameraModeUndefined:     !_cameraPhotoMode && !_cameraVideoMode
    property bool _recordingVideo:          _cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool _isThermal:               TyphoonHQuickInterface.thermalImagePresent && TyphoonHQuickInterface.videoReceiver

    property real _mediaWidth:              128
    property real _mediaHeight:             72
    property real _mediaIndex:              0

    //-- Media Player
    property color  _rectColor:             qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    property string _photoPath:             "file://" + QGroundControl.settingsManager.appSettings.savePath.rawValue.toString() + "/Photo/"
    property bool   _selectMode:            false
    property bool   _hasSelection:          TyphoonHQuickInterface.selectedCount > 0
    property bool   _hasPhotos:             TyphoonHQuickInterface.mediaList.length > 0

    property var    qgcView:                null

    function baseName(str) {
        return (str.slice(str.lastIndexOf("/")+1))
    }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(_communicationLost && rootLoader.sourceComponent === cameraSettingsComponent) {
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
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
            id:             cameraLabel
            text:           _activeVehicle ? (_camera && _camera.modelName !== "" ? _camera.modelName : _commLostStr) : _commLostStr
            font.family:    ScreenTools.demiboldFontFamily
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
                    source:             (_cameraModeUndefined || _cameraVideoMode) ? "/typhoonh/img/camera_switch_video_mode.svg" : (_cameraElapsedMode ? "/typhoonh/img/camera_switch_elapsed_mode.svg" : "/typhoonh/img/camera_switch_photo_mode.svg")
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _cameraModeUndefined ? qgcPal.text : (_camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING ? qgcPal.colorRed : qgcPal.colorGreen)
                }
            }
            MouseArea {
                anchors.fill:   parent
                enabled:        !_cameraModeUndefined && _camera && _camera.videoStatus !== QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING && _cameraPhotoIdle
                onClicked: {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                    _camera.toggleMode()
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
                QGCColoredImage {
                    id:                 busyIndicator
                    height:             parent.height * 0.75
                    width:              height
                    source:             "/qmlimages/MapSync.svg"
                    sourceSize.height:  height
                    fillMode:           Image.PreserveAspectFit
                    mipmap:             true
                    smooth:             true
                    color:              qgcPal.colorBlue
                    visible: {
                        if(_cameraPhotoMode && !_cameraPhotoIdle && !_cameraElapsedMode) {
                            return true
                        }
                        return false
                    }
                    anchors.centerIn:   parent
                    RotationAnimation on rotation {
                        loops:          Animation.Infinite
                        from:           360
                        to:             0
                        duration:       740
                        running:        busyIndicator.visible
                    }
                }
                QGCLabel {
                    text:               _camera ? _camera.photoLapse.toFixed(0) + 's' : 'N/A'
                    font.family:        ScreenTools.demiboldFontFamily
                    color:              qgcPal.colorBlue
                    visible:            _cameraElapsedMode
                    anchors.centerIn:   parent
                }
            }
            Rectangle {
                id:         pauseVideo
                width:      parent.width * 0.5
                height:     width
                color:      _cameraModeUndefined ? qgcPal.colorGrey : qgcPal.colorRed
                visible: {
                   if(_cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) {
                       return true
                   }
                   if(_cameraPhotoMode) {
                       if(_camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_INTERVAL_IDLE || _camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
                           return true
                       }
                   }
                   return false
                }
                anchors.centerIn:   parent
            }
            MouseArea {
                anchors.fill:   parent
                enabled:        !_emptySD
                onClicked: {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                    if(_cameraVideoMode) {
                        if(_camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) {
                            _camera.stopVideo()
                        } else {
                            if(!_fullSD) {
                                _camera.startVideo()
                            }
                        }
                    } else {
                        if(_camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_INTERVAL_IDLE || _camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
                            _camera.stopTakePhoto()
                        } else {
                            if(!_fullSD) {
                                _camera.takePhoto()
                            }
                        }
                    }
                }
            }
        }
        //-- Recording Time
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
            visible: _cameraVideoMode
        }
        QGCLabel {
            text: (_cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) ? _camera.recordTimeStr : "00:00:00"
            visible: _cameraVideoMode
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * _cameraVideoMode ? 0.075 : 0.15
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
            color:              (!_communicationLost && _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED) ? qgcPal.text : qgcPal.colorGrey
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    rootLoader.sourceComponent = mediaPlayerComponent
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
            color:              settingsEnabled ? qgcPal.text : qgcPal.colorGrey
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                enabled:        parent.settingsEnabled
                onClicked: {
                    rootLoader.sourceComponent = cameraSettingsComponent
                }
            }
            property bool settingsEnabled: !_communicationLost && _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED && _cameraPhotoIdle && !_recordingVideo
        }
        Item {
            height:     ScreenTools.defaultFontPixelHeight
            width:      1
        }
    }

    Component {
        id: cameraSettingsComponent
        Item {
            id:     camSettingsItem
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            function showEditFact(fact) {
                factEdit.fact = fact
                factEdit.visible = true
            }
            function hideEditFact() {
                factEdit.visible = false
                factEdit.fact = null
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    mainWindow.enableToolbar()
                    rootLoader.sourceComponent = null
                }
            }
            Rectangle {
                id:             camSettingsShadow
                anchors.fill:   camSettingsRect
                radius:         camSettingsRect.radius
                color:          qgcPal.window
                visible:        false
            }
            DropShadow {
                id:                 camSettingsDS
                anchors.fill:       camSettingsShadow
                visible:            camSettingsRect.visible
                horizontalOffset:   4
                verticalOffset:     4
                radius:             32.0
                samples:            65
                color:              Qt.rgba(0,0,0,0.75)
                source:             camSettingsShadow
            }
            Rectangle {
                id:     camSettingsRect
                width:  mainWindow.width  * 0.65
                height: mainWindow.height * 0.65
                radius: ScreenTools.defaultFontPixelWidth
                color:  qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:   1
                border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn: parent
                MouseArea {
                    anchors.fill:   parent
                    onWheel:        { wheel.accepted = true; }
                    onPressed:      { mouse.accepted = true; }
                    onReleased:     { mouse.accepted = true; }
                }
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
                    anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                    anchors.bottom:     parent.bottom
                    anchors.bottomMargin: ScreenTools.defaultFontPixelHeight
                    width:              cameraSettingsCol.width + (ScreenTools.defaultFontPixelHeight * 2)
                    contentHeight:      cameraSettingsCol.height
                    contentWidth:       cameraSettingsCol.width
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:                 cameraSettingsCol
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                        anchors.margins:    ScreenTools.defaultFontPixelHeight
                        anchors.horizontalCenter: parent.horizontalCenter
                        //-------------------------------------------
                        //-- CGOET Thermal Video Modes
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible:        _camera && _camera.isCGOET
                            property var thermalModes: [qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture")]
                            QGCLabel {
                                text:       qsTr("Thermal View Mode")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCComboBox {
                                width:          _editFieldWidth
                                model:          parent.thermalModes
                                currentIndex:   TyphoonHQuickInterface.thermalMode
                                onActivated:    TyphoonHQuickInterface.thermalMode = index
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _camera && _camera.isCGOET
                        }
                        //-------------------------------------------
                        //-- CGOET Thermal Video Opacity
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible:        _camera && _camera.isCGOET && TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalBlend
                            QGCLabel {
                                text:       qsTr("Blend Opacity")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            YSlider {
                                width:          _editFieldWidth
                                maximumValue:   100
                                minimumValue:   0
                                value:          TyphoonHQuickInterface.thermalOpacity
                                updateValueWhileDragging: true
                                onValueChanged: {
                                    TyphoonHQuickInterface.thermalOpacity = value
                                }
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _camera && _camera.isCGOET && TyphoonHQuickInterface.thermalMode === TyphoonHQuickInterface.ThermalBlend
                        }
                        //-------------------------------------------
                        //-- CGOET Thermal ROI
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible:        _camera && _camera.isCGOET
                            QGCLabel {
                                text:       qsTr("ROI")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            FactComboBox {
                                width:      _editFieldWidth
                                fact:       _camera ? _camera.irROI : null
                                indexModel: false
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _camera && _camera.isCGOET
                        }
                        //-------------------------------------------
                        //-- Settings from Camera Definition File
                        Repeater {
                            model:      _camera ? _camera.activeSettings : []
                            Item {
                                width:   repCol.width
                                height:  repCol.height
                                Column {
                                    id:                 repCol
                                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                    Row {
                                        spacing:        ScreenTools.defaultFontPixelWidth
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        property var    _fact:      _camera.getFact(modelData)
                                        property bool   _isBool:    _fact.typeIsBool
                                        property bool   _isCombo:   !_isBool && _fact.enumStrings.length > 0
                                        property bool   _isSlider:  _fact && !isNaN(_fact.increment)
                                        property bool   _isEdit:    !_isBool && !_isSlider && _fact.enumStrings.length < 1
                                        QGCLabel {
                                            text:       parent._fact.shortDescription
                                            width:      _labelFieldWidth
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        FactComboBox {
                                            width:      parent._isCombo ? _editFieldWidth : 0
                                            fact:       parent._fact
                                            indexModel: false
                                            visible:    parent._isCombo
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                        QGCButton {
                                            visible:    parent._isEdit
                                            width:      parent._isEdit ? _editFieldWidth : 0
                                            text:       parent._fact.valueString
                                            onClicked: {
                                                console.log(parent._fact.shortDescription)
                                                showEditFact(parent._fact)
                                            }
                                        }
                                        YSlider {
                                            width:          parent._isSlider ? _editFieldWidth : 0
                                            maximumValue:   parent._fact.max
                                            minimumValue:   parent._fact.min
                                            stepSize:       parent._fact.increment
                                            visible:        parent._isSlider
                                            updateValueWhileDragging:   false
                                            anchors.verticalCenter:     parent.verticalCenter
                                            Component.onCompleted: {
                                                value = parent._fact.value
                                            }
                                            onValueChanged: {
                                                parent._fact.value = value
                                            }
                                        }
                                        OnOffSwitch {
                                            width:      parent._isBool ? _editFieldWidth : 0
                                            checked:    parent._fact ? parent._fact.value : false
                                            onClicked:  parent._fact.value = checked ? 1 : 0
                                            visible:    parent._isBool
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                    Rectangle {
                                        color:      qgcPal.button
                                        height:     1
                                        width:      cameraSettingsCol.width
                                    }
                                }
                            }
                        }
                        //-------------------------------------------
                        //-- Time Lapse
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible:        _cameraPhotoMode
                            property var photoModes: [qsTr("Single"), qsTr("Time Lapse")]
                            QGCLabel {
                                text:       qsTr("Photo Mode")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCComboBox {
                                width:          _editFieldWidth
                                model:          parent.photoModes
                                currentIndex:   _camera ? _camera.photoMode : 0
                                onActivated:    _camera.photoMode = index
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _cameraPhotoMode
                        }
                        //-------------------------------------------
                        //-- Time Lapse Interval
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            visible:        _cameraPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                            QGCLabel {
                                text:       qsTr("Photo Interval (seconds)")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            YSlider {
                                width:          _editFieldWidth
                                maximumValue:   60
                                minimumValue:   _camera ? (_camera.isE90 ? 3 : 5) : 5
                                stepSize:       1
                                value:          _camera ? _camera.photoLapse : 5
                                updateValueWhileDragging:   true
                                anchors.verticalCenter:     parent.verticalCenter
                                onValueChanged: {
                                    _camera.photoLapse = value
                                }
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _cameraPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                        }
                        //-------------------------------------------
                        //-- Screen Grid
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            visible:        _camera && !_camera.isCGOET
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:       qsTr("Screen Grid")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            OnOffSwitch {
                                checked:     QGroundControl.settingsManager.videoSettings.gridLines.rawValue
                                width:      _editFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked:  QGroundControl.settingsManager.videoSettings.gridLines.rawValue = checked
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                            visible:    _camera && !_camera.isCGOET
                        }
                        //-------------------------------------------
                        //-- Reset Camera
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:       qsTr("Reset Camera Defaults")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       qsTr("Reset")
                                onClicked:  resetPrompt.open()
                                width:      _editFieldWidth
                                enabled:    !_recordingVideo
                                anchors.verticalCenter: parent.verticalCenter
                                MessageDialog {
                                    id:                 resetPrompt
                                    title:              qsTr("Reset Camera to Factory Settings")
                                    text:               qsTr("Confirm resetting all settings?")
                                    standardButtons:    StandardButton.Yes | StandardButton.No
                                    onNo: resetPrompt.close()
                                    onYes: {
                                        _camera.resetSettings()
                                        QGroundControl.settingsManager.videoSettings.gridLines.rawValue = false
                                        TyphoonHQuickInterface.thermalMode = TyphoonHQuickInterface.ThermalBlend
                                        TyphoonHQuickInterface.thermalOpacity = 85
                                        _camera.photoMode = QGCCameraControl.PHOTO_CAPTURE_SINGLE
                                        _camera.photoLapse = 5.0
                                        _camera.photoLapseCount = 0
                                        resetPrompt.close()
                                    }
                                }
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                        }
                        //-------------------------------------------
                        //-- Format SD Card
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:       qsTr("Micro SD Card")
                                width:      _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCButton {
                                text:       qsTr("Format")
                                enabled:    !_emptySD && !_recordingVideo
                                onClicked:  formatPrompt.open()
                                width:      _editFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                                MessageDialog {
                                    id:                 formatPrompt
                                    title:              qsTr("Format MicroSD Card")
                                    text:               qsTr("Confirm erasing all files?")
                                    standardButtons:    StandardButton.Yes | StandardButton.No
                                    onNo: formatPrompt.close()
                                    onYes: {
                                        _camera.formatCard()
                                        formatPrompt.close()
                                    }
                                }
                            }
                        }
                        Rectangle {
                            color:      qgcPal.button
                            height:     1
                            width:      cameraSettingsCol.width
                        }
                    }
                }
            }
            Rectangle {
                id:             factEdit
                visible:        false
                color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
                anchors.fill:   parent
                property var fact: null
                MouseArea {
                    anchors.fill:   parent
                    onWheel:        { wheel.accepted = true; }
                    onPressed:      { mouse.accepted = true; }
                    onReleased:     { mouse.accepted = true; }
                }
                Rectangle {
                    width:      factEditCol.width  * 1.25
                    height:     factEditCol.height * 1.25
                    color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                    border.width:       1
                    border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                    anchors.top:        parent.top
                    anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:             factEditCol
                        spacing:        ScreenTools.defaultFontPixelHeight
                        anchors.centerIn: parent
                        QGCLabel {
                            text:       factEdit.fact ? factEdit.fact.shortDescription : ""
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        YTextField {
                            id:         factEditor
                            width:      _editFieldWidth
                            fact:       factEdit.fact
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCButton {
                            text: qsTr("Close")
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                console.log('Done with ' + factEdit.fact.shortDescription)
                                factEditor.completeEditing()
                                hideEditFact()
                            }
                        }
                    }
                }
            }
            Component.onCompleted: {
                rootLoader.width  = camSettingsItem.width
                rootLoader.height = camSettingsItem.height
            }
            Keys.onBackPressed: {
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
            }
        }
    }
    //-- Vehicle did not report any camera
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
                        onClicked: {
                            mainWindow.enableToolbar()
                            rootLoader.sourceComponent = null
                        }
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
                    mainWindow.enableToolbar()
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
                width:              mediaPlayerGrid.width  + (ScreenTools.defaultFontPixelHeight * 2)
                height:             mediaPlayerGrid.height + (ScreenTools.defaultFontPixelHeight * 5)
                radius:             ScreenTools.defaultFontPixelWidth
                color:              _rectColor
                border.width:       1
                border.color:       qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
                anchors.centerIn:   parent
                MouseArea {
                    anchors.fill:   parent
                    onWheel:        { wheel.accepted = true; }
                    onPressed:      { mouse.accepted = true; }
                    onReleased:     { mouse.accepted = true; }
                }
                QGCLabel {
                    anchors.top:            parent.top
                    anchors.topMargin:      ScreenTools.defaultFontPixelHeight
                    anchors.left:           parent.left
                    anchors.leftMargin:     ScreenTools.defaultFontPixelHeight * 2
                    height:                 buttonRow.height
                    verticalAlignment:      Text.AlignVCenter
                    text:                   qsTr("Local Storage")
                }
                Row {
                    id:                     buttonRow
                    spacing:                ScreenTools.defaultFontPixelWidth
                    anchors.top:            parent.top
                    anchors.topMargin:      ScreenTools.defaultFontPixelHeight
                    anchors.right:          parent.right
                    anchors.rightMargin:    ScreenTools.defaultFontPixelHeight * 2
                    QGCButton {
                        text:           qsTr('Delete Selected')
                        width:          ScreenTools.defaultFontPixelWidth * 15
                        visible:        _selectMode
                        enabled:        _hasSelection
                        onClicked: {
                            confirmDeleteAll.open()
                        }
                        MessageDialog {
                            id:                 confirmDeleteAll
                            title:              qsTr("Delete All Images")
                            text:               qsTr("Confirm deleting selected images?")
                            standardButtons:    StandardButton.Ok | StandardButton.Cancel
                            onAccepted: {
                                TyphoonHQuickInterface.deleteSelectedMedia()
                                confirmDeleteAll.close()
                            }
                        }
                    }
                    QGCButton {
                        text:           _hasSelection ? qsTr('Select None') : qsTr('Select All')
                        width:          ScreenTools.defaultFontPixelWidth * 15
                        enabled:        _hasPhotos
                        visible:        _selectMode
                        onClicked: {
                            if(!_hasSelection) {
                                _selectMode = true
                            }
                            TyphoonHQuickInterface.selectAllMedia(!_hasSelection)
                        }
                    }
                    QGCButton {
                        text:           _selectMode ? qsTr('Cancel') : qsTr('Select')
                        width:          ScreenTools.defaultFontPixelWidth * 15
                        enabled:        _hasPhotos
                        onClicked:      {
                            if(_selectMode) {
                                // Clear selection
                                TyphoonHQuickInterface.selectAllMedia(false)
                                _selectMode = false
                            } else {
                                _selectMode = true
                            }
                        }
                    }
                }
                GridView {
                    id:                 mediaPlayerGrid
                    clip:               true
                    model:              TyphoonHQuickInterface.mediaList
                    cellWidth:          _mediaWidth  + 8
                    cellHeight:         _mediaHeight + 8
                    width:              (_mediaWidth  + 8) * 5
                    height:             (_mediaHeight + 8) * 4
                    delegate:           mediaDelegate
                    cacheBuffer:        height * 4
                    anchors.bottom:     parent.bottom
                    anchors.bottomMargin:       ScreenTools.defaultFontPixelHeight
                    anchors.horizontalCenter:   parent.horizontalCenter
                }
                QGCLabel {
                    text:       qsTr("No Images")
                    visible:    TyphoonHQuickInterface.mediaList.length < 1
                    anchors.centerIn: parent
                }
            }
            Component.onCompleted: {
                _selectMode = false
                TyphoonHQuickInterface.refreshMeadiaList()
                rootLoader.width  = mediaPlayerItem.width
                rootLoader.height = mediaPlayerItem.height
            }
            Keys.onBackPressed: {
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
            }
        }
    }
    Component {
        id: mediaDelegate
        Rectangle {
            width:          _mediaWidth
            height:         _mediaHeight
            color:          Qt.rgba(0,0,0,0)
            border.color:   _selectMode ? (_mediaItem && _mediaItem.selected ? qgcPal.colorGreen : qgcPal.colorGrey) : _rectColor
            border.width:   _selectMode ? (_mediaItem && _mediaItem.selected ? 2 : 1) : 0
            property var _mediaItem: _hasPhotos ? TyphoonHQuickInterface.mediaList[index] : null
            Image {
                anchors.fill:       parent
                anchors.margins:    2
                fillMode:           Image.PreserveAspectFit
                source:             _mediaItem ? _photoPath + _mediaItem.fileName : ""
                sourceSize.width:   width
                opacity:            _selectMode && _mediaItem && !_mediaItem.selected ? 0.5 : 1
                MouseArea {
                    anchors.fill:   parent
                    onClicked: {
                        if(_selectMode) {
                            if(_mediaItem) {
                                _mediaItem.selected = !_mediaItem.selected
                            }
                        } else {
                            _mediaIndex = index
                            rootLoader.sourceComponent = mediaViewComponent
                        }
                    }
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
            property var _mediaItem: _hasPhotos ? TyphoonHQuickInterface.mediaList[_mediaIndex] : null
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
                    width:          _mediaWidth  * 7
                    height:         _mediaHeight * 7
                    fillMode:       Image.PreserveAspectFit
                    source:         _mediaItem ? _photoPath + _mediaItem.fileName : ""
                    cache:          false
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
                    visible:            _mediaIndex < (TyphoonHQuickInterface.mediaList.length - 1)
                    color:              qgcPal.text
                    MouseArea {
                        anchors.fill:   parent
                        onClicked: {
                            if(_mediaIndex < (TyphoonHQuickInterface.mediaList.length - 1)) {
                                _mediaIndex = _mediaIndex + 1;
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
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
            }
        }
    }
}

