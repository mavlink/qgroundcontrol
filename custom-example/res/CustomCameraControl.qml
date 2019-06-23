/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                  2.11
import QtQuick.Controls         2.4
import QtQuick.Layouts          1.11
import QtQuick.Dialogs          1.3

import QtMultimedia             5.9
import QtPositioning            5.2

import QGroundControl                   1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Vehicle           1.0

import CustomQuickInterface             1.0
import Custom.Widgets                   1.0

Item {
    height:         cameraRect.height
    width:          cameraRect.width + (ScreenTools.defaultFontPixelWidth * 2)
    visible:        !QGroundControl.videoManager.fullScreen

    readonly property string _commLostStr: qsTr("NO CAMERA")

    property real   _spacers:               ScreenTools.defaultFontPixelHeight
    property real   _labelFieldWidth:       ScreenTools.defaultFontPixelWidth * 28
    property real   _editFieldWidth:        ScreenTools.defaultFontPixelWidth * 30

    property var    _dynamicCameras:        activeVehicle ? activeVehicle.dynamicCameras : null
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property int    _curCameraIndex:        _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _communicationLost:     activeVehicle ? activeVehicle.connectionLost : false
    property bool   _noSdCard:              _camera && _camera.storageTotal === 0
    property bool   _fullSD:                _camera && _camera.storageTotal !== 0 && _camera.storageFree > 0 && _camera.storageFree < 250 // We get kiB from the camera
    property bool   _cameraVideoMode:       !_communicationLost && (_noSdCard ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_VIDEO)
    property bool   _cameraPhotoMode:       !_communicationLost && (_noSdCard ? false : _camera && (_camera.cameraMode  === QGCCameraControl.CAM_MODE_PHOTO || _camera.cameraMode === QGCCameraControl.CAM_MODE_SURVEY))
    property bool   _cameraPhotoIdle:       !_communicationLost && (_noSdCard ? false : _camera && _camera.photoStatus  === QGCCameraControl.PHOTO_CAPTURE_IDLE)
    property bool   _cameraElapsedMode:     !_communicationLost && (_noSdCard ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_PHOTO && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE)
    property bool   _cameraModeUndefined:   !_cameraPhotoMode && !_cameraVideoMode
    property bool   _recordingVideo:        _cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool   _settingsEnabled:       !_communicationLost && _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED && _camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_IDLE && !_recordingVideo
    property bool   _hasZoom:               _camera && _camera.hasZoom

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(_communicationLost && cameraSettings.visible) {
                cameraSettings.close()
            }
        }
    }

    DeadMouseArea {
        anchors.fill:   parent
    }

    Rectangle {
        id:             cameraRect
        height:         cameraCol.height
        width:          cameraCol.width + (ScreenTools.defaultFontPixelWidth * 4)
        color:          qgcPal.windowShade
        radius:         ScreenTools.defaultFontPixelWidth * 0.5
        Column {
            id:         cameraCol
            spacing:    _spacers
            anchors.centerIn: parent
            Item {
                height:     1
                width:      1
            }
            //-----------------------------------------------------------------
            //-- Camera Name
            QGCLabel {
                text:                   activeVehicle ? (_camera && _camera.modelName !== "" ? _camera.modelName : _commLostStr) : _commLostStr
                font.pointSize:         ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            //-----------------------------------------------------------------
            //-- Camera Mode
            Item {
                width:                  modeCol.width
                height:                 modeCol.height
                anchors.horizontalCenter: parent.horizontalCenter
                Column {
                    id:                 modeCol
                    spacing:            _spacers * 0.5
                    QGCColoredImage {
                        height:         ScreenTools.defaultFontPixelHeight * 1.25
                        width:          height
                        source:         (_cameraModeUndefined || _cameraPhotoMode) ? "/custom/img/camera_photo.svg" : "/custom/img/camera_video.svg"
                        color:          qgcPal.text
                        fillMode:       Image.PreserveAspectFit
                        sourceSize.height:  height
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:           _cameraVideoMode ? qsTr("Video") : qsTr("Photo")
                        font.pointSize: ScreenTools.smallFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                MouseArea {
                    anchors.fill:       parent
                    enabled:            !_cameraModeUndefined && _camera && _camera.videoStatus !== QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING && _cameraPhotoIdle
                    onClicked: {
                        _camera.toggleMode()
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Shutter
            Rectangle {
                color:                  Qt.rgba(0,0,0,0)
                width:                  height
                height:                 ScreenTools.defaultFontPixelHeight * 4
                radius:                 width * 0.5
                border.color:           qgcPal.buttonText
                border.width:           2
                anchors.horizontalCenter: parent.horizontalCenter
                Rectangle {
                    width:              parent.width * 0.75
                    height:             width
                    radius:             width * 0.5
                    color:              _cameraModeUndefined ? qgcPal.colorGrey : ( _cameraVideoMode ? qgcPal.colorRed : qgcPal.text )
                    visible:            !pauseVideo.visible
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
                        color:              qgcPal.windowShade
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
                        text:               _camera ? _camera.photoLapse.toFixed(0) + 's' : qsTr('N/A')
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
                    enabled:        !_noSdCard
                    onClicked: {
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
            //-----------------------------------------------------------------
            //-- Settings
            Item {
                width:                  settingsCol.width
                height:                 settingsCol.height
                anchors.horizontalCenter: parent.horizontalCenter
                Column {
                    id:                 settingsCol
                    spacing:            _spacers * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCColoredImage {
                        width:                      ScreenTools.defaultFontPixelHeight * 1.25
                        height:                     width
                        sourceSize.width:           width
                        source:                     "qrc:/custom/img/camera_settings.svg"
                        color:                      qgcPal.text
                        fillMode:                   Image.PreserveAspectFit
                        opacity:                    _settingsEnabled ? 1 : 0.5
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }
                    QGCLabel {
                        text:                       qsTr("Settings")
                        font.pointSize:             ScreenTools.smallFontPointSize
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }
                }
                MouseArea {
                    anchors.fill:       parent
                    enabled:            _settingsEnabled
                    onClicked: {
                        cameraSettings.open()
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- microSD Card
            Column {
                spacing:                        _spacers * 0.5
                anchors.horizontalCenter:       parent.horizontalCenter
                QGCColoredImage {
                    width:                      ScreenTools.defaultFontPixelHeight * 1.25
                    height:                     width
                    sourceSize.width:           width
                    source:                     "qrc:/custom/img/microSD.svg"
                    color:                      qgcPal.text
                    fillMode:                   Image.PreserveAspectFit
                    opacity:                    _settingsEnabled ? 1 : 0.5
                    anchors.horizontalCenter:   parent.horizontalCenter
                }
                QGCLabel {
                    text: {
                        if(_noSdCard) return qsTr("NONE")
                        if(_fullSD) return qsTr("FULL")
                        return _camera ? _camera.storageFreeStr : ""
                    }
                    color:          (_noSdCard || _fullSD) ? qgcPal.colorOrange : qgcPal.text
                    font.pointSize: ScreenTools.smallFontPointSize
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            /*
            //-----------------------------------------------------------------
            //-- Recording Time / Images Captured
            CustomLabel {
                text:               (_cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) ? _camera.recordTimeStr : "00:00:00"
                visible:            _cameraVideoMode
                pointSize:          ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            CustomLabel {
                text:               activeVehicle && _cameraPhotoMode ? ('00000' + activeVehicle.cameraTriggerPoints.count).slice(-5) : "00000"
                visible:            _cameraPhotoMode
                pointSize:          ScreenTools.smallFontPointSize
                anchors.horizontalCenter: parent.horizontalCenter
            }
            */
            Item {
                height:     1
                width:      1
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Camera Settings
    Popup {
        id:                 cameraSettings
        width:              Math.min(mainWindow.width  * 0.666, ScreenTools.defaultFontPixelWidth * 80)
        height:             mainWindow.height * 0.666
        modal:              true
        focus:              true
        parent:             Overlay.overlay
        x:                  Math.round((mainWindow.width  - width)  * 0.5)
        y:                  Math.round((mainWindow.height - height) * 0.5)
        closePolicy:        Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
            border.color:   qgcPal.text
            radius:         ScreenTools.defaultFontPixelWidth
        }
        Item {
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            function showEditFact(fact) {
                factEditor.text = fact.valueString
                factEdit.fact = fact
                factEdit.visible = true
            }
            function hideEditFact() {
                factEdit.visible = false
                factEdit.fact = null
            }
            QGCLabel {
                id:                 cameraSettingsLabel
                text:               _noSdCard ? qsTr("Settings") : (_cameraVideoMode ? qsTr("Video Settings") : qsTr("Photo Settings"))
                font.family:        ScreenTools.demiboldFontFamily
                font.pointSize:     ScreenTools.mediumFontPointSize
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.top:        parent.top
                anchors.left:       parent.left
            }
            QGCFlickable {
                clip:               true
                anchors.top:        cameraSettingsLabel.bottom
                anchors.bottom:     parent.bottom
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                width:              cameraSettingsCol.width + (ScreenTools.defaultFontPixelWidth * 2)
                contentHeight:      cameraSettingsCol.height
                contentWidth:       cameraSettingsCol.width
                anchors.horizontalCenter: parent.horizontalCenter
                Column {
                    id:                 cameraSettingsCol
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.horizontalCenter: parent.horizontalCenter
                    //-------------------------------------------
                    //-- Camera Selector
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        visible:        _isCamera && _dynamicCameras.cameraLabels.length > 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:           qsTr("Camera Selector:")
                            width:          _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCComboBox {
                            model:          _isCamera ? _dynamicCameras.cameraLabels : []
                            width:          _editFieldWidth
                            onActivated:    _dynamicCameras.currentCamera = index
                            currentIndex:   _dynamicCameras ? _dynamicCameras.currentCamera : 0
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:    _isCamera && _dynamicCameras.cameraLabels.length > 1
                    }
                    //-------------------------------------------
                    //-- Stream Selector
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        visible:        _isCamera && _camera.streamLabels.length > 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:           qsTr("Stream Selector:")
                            width:          _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCComboBox {
                            model:          _camera ? _camera.streamLabels : []
                            width:          _editFieldWidth
                            onActivated:    _camera.currentStream = index
                            currentIndex:   _camera ? _camera.currentStream : 0
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:    _isCamera && _camera.streamLabels.length > 1
                    }
                    //-------------------------------------------
                    //-- Thermal Modes
                    Row {
                        spacing:            ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible:            QGroundControl.videoManager.hasThermal
                        property var thermalModes: [qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture")]
                        QGCLabel {
                            text:           qsTr("Thermal View Mode")
                            width:          _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCComboBox {
                            width:          _editFieldWidth
                            model:          parent.thermalModes
                            currentIndex:   _camera ? _camera.thermalMode : 0
                            onActivated:    _camera.thermalMode = index
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:            QGroundControl.videoManager.hasThermal
                    }
                    //-------------------------------------------
                    //-- Thermal Video Opacity
                    Row {
                        spacing:            ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible:            QGroundControl.videoManager.hasThermal && _camera.thermalMode === QGCCameraControl.THERMAL_BLEND
                        QGCLabel {
                            text:           qsTr("Blend Opacity")
                            width:          _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Slider {
                            width:          _editFieldWidth
                            to:             100
                            from:           0
                            value:          _camera ? _camera.thermalOpacity : 0
                            live:           true
                            onValueChanged: {
                                _camera.thermalOpacity = value
                            }
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:            QGroundControl.videoManager.hasThermal && _camera.thermalMode === QGCCameraControl.THERMAL_BLEND
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
                                property var _fact: _camera.getFact(modelData)
                                Row {
                                    height:         visible ? undefined : 0
                                    spacing:        ScreenTools.defaultFontPixelWidth
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    property bool   _isBool:    parent._fact.typeIsBool
                                    property bool   _isCombo:   !_isBool && parent._fact.enumStrings.length > 0
                                    property bool   _isSlider:  parent._fact && !isNaN(parent._fact.increment)
                                    property bool   _isEdit:    !_isBool && !_isSlider && parent._fact.enumStrings.length < 1
                                    QGCLabel {
                                        text:       parent.parent._fact.shortDescription
                                        width:      _labelFieldWidth
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    FactComboBox {
                                        width:      parent._isCombo ? _editFieldWidth : 0
                                        fact:       parent.parent._fact
                                        indexModel: false
                                        visible:    parent._isCombo
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    QGCButton {
                                        visible:    parent._isEdit
                                        width:      parent._isEdit ? _editFieldWidth : 0
                                        text:       parent.parent._fact.valueString
                                        onClicked: {
                                            showEditFact(parent.parent._fact)
                                        }
                                    }
                                    QGCSlider {
                                        width:          parent._isSlider ? _editFieldWidth : 0
                                        maximumValue:   parent.parent._fact.max
                                        minimumValue:   parent.parent._fact.min
                                        stepSize:       parent.parent._fact.increment
                                        visible:        parent._isSlider
                                        updateValueWhileDragging:   false
                                        anchors.verticalCenter:     parent.verticalCenter
                                        Component.onCompleted: {
                                            value = parent.parent._fact.value
                                        }
                                        onValueChanged: {
                                            parent.parent._fact.value = value
                                        }
                                    }
                                    CustomOnOffSwitch {
                                        width:      parent._isBool ? _editFieldWidth : 0
                                        checked:    parent.parent._fact ? parent.parent._fact.value : false
                                        onClicked:  parent.parent._fact.value = checked ? 1 : 0
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
                        visible:        _cameraPhotoMode && !_noSdCard
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
                        visible:    _cameraPhotoMode && !_noSdCard
                    }
                    //-------------------------------------------
                    //-- Time Lapse Interval
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        anchors.horizontalCenter: parent.horizontalCenter
                        visible:        _cameraPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE && !_noSdCard
                        QGCLabel {
                            text:       qsTr("Photo Interval (seconds)")
                            width:      _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCSlider {
                            width:          _editFieldWidth
                            maximumValue:   60
                            minimumValue:   _camera ? (_camera.isE90 ? 3 : 5) : 5
                            stepSize:       1
                            value:          _camera ? _camera.photoLapse : 5
                            updateValueWhileDragging:   true
                            anchors.verticalCenter:     parent.verticalCenter
                            onValueChanged: {
                                if(_camera) {
                                    _camera.photoLapse = value
                                }
                            }
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:    _cameraPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE && !_noSdCard
                    }
                    //-------------------------------------------
                    //-- Screen Grid
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        visible:        _camera && !_camera.isThermal
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:       qsTr("Screen Grid")
                            width:      _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        CustomOnOffSwitch {
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
                        visible:    _camera && !_camera.isThermal
                    }
                    //-------------------------------------------
                    //-- Video Fit
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        visible:        _camera
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:       qsTr("Video Screen Fit")
                            width:      _labelFieldWidth
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        FactComboBox {
                            width:      _editFieldWidth
                            fact:       QGroundControl.settingsManager.videoSettings.videoFit
                            indexModel: false
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Rectangle {
                        color:      qgcPal.button
                        height:     1
                        width:      cameraSettingsCol.width
                        visible:    _camera && !_camera.isThermal
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
                }
            }
            Rectangle {
                id:             factEdit
                visible:        false
                color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5)
                anchors.fill:   parent
                property var fact: null
                DeadMouseArea {
                    anchors.fill:   parent
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
                        FactTextField {
                            id:         factEditor
                            width:      _editFieldWidth
                            fact:       factEdit.fact
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCButton {
                            text: qsTr("Close")
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                factEditor.completeEditing()
                                hideEditFact()
                            }
                        }
                    }
                }
            }
        }
    }
}

