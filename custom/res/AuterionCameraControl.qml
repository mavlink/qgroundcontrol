/****************************************************************************
 *
 * (c) 2009-2018 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.9
import QtQuick.Controls         2.2
import QtMultimedia             5.9
import QtPositioning            5.2
import QtQuick.Layouts          1.2
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

import AuterionQuickInterface           1.0
import Auterion.Widgets                 1.0

Rectangle {
    id:             mainRect
    height:         mainCol.height
    width:          _indicatorDiameter
    visible:        !QGroundControl.videoManager.fullScreen
    radius:         ScreenTools.defaultFontPixelWidth * 0.5
    color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    border.width:   1
    border.color:   _borderColor

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    readonly property string _commLostStr: qsTr("NO CAMERA")

    property real _spacers:                 ScreenTools.defaultFontPixelHeight * 0.35
    property real _labelFieldWidth:         ScreenTools.defaultFontPixelWidth * 36
    property real _editFieldWidth:          ScreenTools.defaultFontPixelWidth * 30

    property color  _borderColor:           AuterionQuickInterface.borderColor
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _dynamicCameras:        _activeVehicle ? _activeVehicle.dynamicCameras : null
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property int    _curCameraIndex:        _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property var    _camera:                _isCamera ? _dynamicCameras.cameras.get(_curCameraIndex) : null
    property bool   _communicationLost:     _activeVehicle ? _activeVehicle.connectionLost : false
    property bool   _noSdCard:              _camera && _camera.storageTotal === 0
    property bool   _cameraVideoMode:       !_communicationLost && (_noSdCard ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_VIDEO)
    property bool   _cameraPhotoMode:       !_communicationLost && (_noSdCard ? false : _camera && (_camera.cameraMode  === QGCCameraControl.CAM_MODE_PHOTO || _camera.cameraMode === QGCCameraControl.CAM_MODE_SURVEY))
    property bool   _cameraPhotoIdle:       !_communicationLost && (_noSdCard ? false : _camera && _camera.photoStatus  === QGCCameraControl.PHOTO_CAPTURE_IDLE)
    property bool   _cameraElapsedMode:     !_communicationLost && (_noSdCard ? false : _camera && _camera.cameraMode   === QGCCameraControl.CAM_MODE_PHOTO && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE)
    property bool   _cameraModeUndefined:   !_cameraPhotoMode && !_cameraVideoMode
    property bool   _recordingVideo:        _cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool   _settingsEnabled:       !_communicationLost && _camera && _camera.cameraMode !== QGCCameraControl.CAM_MODE_UNDEFINED && _camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_IDLE && !_recordingVideo
    property Fact   _emptyfact:             Fact { }

    Connections {
        target: QGroundControl.multiVehicleManager.activeVehicle
        onConnectionLostChanged: {
            if(_communicationLost && rootLoader.sourceComponent === cameraSettingsComponent) {
                mainWindow.enableToolbar()
                rootLoader.sourceComponent = null
            }
        }
    }

    DeadMouseArea {
        anchors.fill:   parent
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
                source:             (_cameraModeUndefined || _cameraVideoMode) ? "/auterion/img/camera_switch_video.svg" : "/auterion/img/camera_switch_photo.svg"
                fillMode:           Image.PreserveAspectFit
                sourceSize.height:  height
                color:              qgcPal.text
                QGCColoredImage {
                    anchors.fill:       parent
                    source:             (_cameraModeUndefined || _cameraVideoMode) ? "/auterion/img/camera_switch_video_mode.svg" : (_cameraElapsedMode ? "/auterion/img/camera_switch_elapsed_mode.svg" : "/auterion/img/camera_switch_photo_mode.svg")
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
        //-- Recording Time / Images Captured
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
        }
        QGCLabel {
            text: (_cameraVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) ? _camera.recordTimeStr : "00:00:00"
            visible: _cameraVideoMode
            anchors.horizontalCenter: parent.horizontalCenter
        }
        QGCLabel {
            text: _activeVehicle && _cameraPhotoMode ? ('00000' + _activeVehicle.cameraTriggerPoints.count).slice(-5) : "00000"
            visible: _cameraPhotoMode
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Item {
            width:  1
            height: ScreenTools.defaultFontPixelHeight * 0.15
        }
        Rectangle {
            height: 1
            width:  parent.width * 0.85
            color:  qgcPal.globalTheme === QGCPalette.Dark ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.25)
            anchors.horizontalCenter: parent.horizontalCenter
        }
        //-- Settings
        QGCColoredImage {
            width:              ScreenTools.defaultFontPixelHeight * 2.5
            height:             width
            sourceSize.width:   width
            source:             "qrc:/auterion/img/sliders.svg"
            fillMode:           Image.PreserveAspectFit
            color:              _settingsEnabled ? qgcPal.text : qgcPal.colorGrey
            anchors.horizontalCenter: parent.horizontalCenter
            MouseArea {
                anchors.fill:   parent
                enabled:        _settingsEnabled
                onClicked: {
                    rootLoader.sourceComponent = cameraSettingsComponent
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
            id:     camSettingsItem
            width:  mainWindow.width
            height: mainWindow.height
            anchors.centerIn: parent
            function showEditFact(fact) {
                factEditor.text = fact.valueString
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
                id:                 camSettingsRect
                width:              mainWindow.width  * 0.65
                height:             mainWindow.height * 0.65
                radius:             ScreenTools.defaultFontPixelWidth
                color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
                border.width:       1
                border.color:       _borderColor
                anchors.centerIn:   parent
                DeadMouseArea {
                    anchors.fill:   parent
                }
                QGCLabel {
                    id:                 cameraSettingsLabel
                    text:               _noSdCard ? qsTr("Settings") : (_cameraVideoMode ? qsTr("Video Settings") : qsTr("Photo Settings"))
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
                        //-- Camera Selector
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCLabel {
                                text:           qsTr("Camera Selector:")
                                width:          _labelFieldWidth
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QGCComboBox {
                                id:             cameraSelector
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
                                        OnOffSwitch {
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
                                    _camera.photoLapse = value
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
                            visible:    _camera && !_camera.isThermal
                        }
                        //-------------------------------------------
                        //-- Video Fit
                        Row {
                            spacing:        ScreenTools.defaultFontPixelWidth
                            visible:        _camera && !_camera.isThermal
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
}

