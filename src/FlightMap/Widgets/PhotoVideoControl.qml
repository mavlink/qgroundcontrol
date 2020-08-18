/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.4
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         1.4
import QtQuick.Dialogs          1.2
import QtGraphicalEffects       1.0

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0

Rectangle {
    height:     mainLayout.height + (_margins * 2)
    color:      "#80000000"
    radius:     _margins
    visible:    !QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.rawValue && (_camera || _anyVideoStreamAvailable) && multiVehiclePanelSelector.showSingleVehiclePanel

    property real   _margins:                               ScreenTools.defaultFontPixelHeight / 2
    property var    _activeVehicle:                         QGroundControl.multiVehicleManager.activeVehicle
    property var    _cameraManager:                         _activeVehicle ? _activeVehicle.cameraManager : null
    property var    _videoManager:                          QGroundControl.videoManager
    property bool   _noCameras:                             _cameraManager ? _cameraManager.cameras.count === 0 : true
    property bool   _multipleCameras:                       _cameraManager ? _cameraManager.cameras.count > 1 : false
    property bool   _noMavlinkCameraStreams:                _camera ? _camera.streamLabels.length : true
    property bool   _multipleMavlinkCameraStreams:          _camera ? _camera.streamLabels.length > 1 : false
    property bool   _anyVideoStreamAvailable:               _videoManager.hasVideo
    property int    _curCameraIndex:                        _cameraManager ? _cameraManager.currentCamera : -1
    property int    _curStreamIndex:                        _camera ? _camera.currentStream : -1
    property var    _camera:                                !_noCameras ? (_cameraManager.cameras.get(_curCameraIndex) && _cameraManager.cameras.get(_curCameraIndex).paramComplete ? _cameraManager.cameras.get(_curCameraIndex) : null) : null
    property string _cameraName:                            _camera ? (_multipleCameras ? _camera.modelName : "") : qsTr("Video Stream")
    property bool   _hasThermalVideoStream:                 _camera ? _camera.thermalStreamInstance : false
    property bool   _cameraModeUndefined:                   _camera ? _camera.cameraMode === QGCCameraControl.CAM_MODE_UNDEFINED : true
    property bool   _cameraInVideoMode:                     _camera ? _camera.cameraMode === QGCCameraControl.CAM_MODE_VIDEO : false
    property bool   _cameraInPhotoMode:                     _camera ? _camera.cameraMode === QGCCameraControl.CAM_MODE_PHOTO : false
    property bool   _cameraElapsedMode:                     _camera && _camera.cameraMode === QGCCameraControl.CAM_MODE_PHOTO && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
    property real   _spacers:                               ScreenTools.defaultFontPixelHeight * 0.5
    property real   _labelFieldWidth:                       ScreenTools.defaultFontPixelWidth * 30
    property real   _editFieldWidth:                        ScreenTools.defaultFontPixelWidth * 30
    property bool   _communicationLost:                     _activeVehicle ? _activeVehicle.connectionLost : false
    property bool   _hasModes:                              _camera && _camera.hasModes
    property bool   _videoRecording:                        _camera && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING
    property bool   _photoIdle:                             _camera && (_camera.photoStatus === QGCCameraControl.PHOTO_CAPTURE_IDLE || _camera.photoStatus >= QGCCameraControl.PHOTO_CAPTURE_LAST)
    property bool   _storageReady:                          _camera && _camera.storageStatus === QGCCameraControl.STORAGE_READY
    property bool   _batteryReady:                          _camera && _camera.batteryRemaining >= 0
    property bool   _storageSupported:                      _camera && _camera.storageStatus === QGCCameraControl.STORAGE_NOT_SUPPORTED
    property bool   _canShoot:                              (!_cameraModeUndefined && ((_storageReady && _camera.storageFree > 0) || _storageSupported)) || _videoManager.streaming
    property bool   _isShooting:                            ((_cameraInVideoMode && _videoRecording) || (_cameraInPhotoMode && !_photoIdle)) || _videoManager.recording
    property var    _videoSettings:                         QGroundControl.settingsManager.videoSettings

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCColoredImage {
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.right:      parent.right
        source:             "/res/gear-black.svg"
        mipmap:             true
        height:             ScreenTools.defaultFontPixelHeight
        width:              height
        sourceSize.height:  height
        color:              qgcPal.text
        fillMode:           Image.PreserveAspectFit

        QGCMouseArea {
            fillItem:   parent
            onClicked:  mainWindow.showPopupDialogFromComponent(settingsDialogComponent)
        }
    }

    ColumnLayout {
        id:                         mainLayout
        anchors.margins:            _margins
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight / 2

        //-- Photo/Video Mode Selector (Mavlink Cameras only)
        Rectangle {
            id:                 camMode
            Layout.alignment:   Qt.AlignHCenter
            width:              _hasModes ? ScreenTools.defaultFontPixelWidth * 10 : 0
            height:             _hasModes ? width / 2 : 0
            color:              qgcPal.windowShadeLight
            radius:             height * 0.5
            visible:            _hasModes

            //-- Video Mode
            Rectangle {
                width:  parent.height
                height: parent.height
                color:  _cameraInVideoMode ? qgcPal.window : qgcPal.windowShadeLight
                radius: height * 0.5
                anchors.left: parent.left
                border.color: qgcPal.text
                border.width: _cameraInVideoMode ? 1 : 0
                anchors.verticalCenter: parent.verticalCenter
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/qmlimages/camera_video.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _cameraInVideoMode ? qgcPal.colorGreen : qgcPal.text
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        _cameraInPhotoMode && !_isShooting
                        onClicked: {
                            _camera.setVideoMode()
                        }
                    }
                }
            }
            //-- Photo Mode
            Rectangle {
                width:  parent.height
                height: parent.height
                color:  _cameraInPhotoMode ? qgcPal.window : qgcPal.windowShadeLight
                radius: height * 0.5
                anchors.right: parent.right
                border.color: qgcPal.text
                border.width: _cameraInPhotoMode ? 1 : 0
                anchors.verticalCenter: parent.verticalCenter
                QGCColoredImage {
                    height:             parent.height * 0.5
                    width:              height
                    anchors.centerIn:   parent
                    source:             "/qmlimages/camera_photo.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    color:              _cameraInPhotoMode ? qgcPal.colorGreen : qgcPal.text
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        _cameraInVideoMode && !_isShooting
                        onClicked: {
                            _camera.setPhotoMode()
                        }
                    }
                }
            }
        }

        // Take Photo, Start/Stop Video button
        Rectangle {
            Layout.alignment:   Qt.AlignHCenter
            color:              Qt.rgba(0,0,0,0)
            width:              ScreenTools.defaultFontPixelWidth * 6
            height:             width
            radius:             width * 0.5
            visible:            _camera || _anyVideoStreamAvailable
            border.color:       qgcPal.buttonText
            border.width:       3

            Rectangle {
                anchors.centerIn:   parent
                width:              parent.width * (_isShooting ? 0.5 : 0.75)
                height:             width
                radius:             _isShooting ? 0 : width * 0.5
                color:              _canShoot ? qgcPal.colorRed : qgcPal.colorGrey
            }

            MouseArea {
                anchors.fill:   parent
                enabled:        _canShoot
                onClicked: {
                    if (_camera) {
                        if(_cameraInVideoMode) {
                            _camera.toggleVideo()
                        } else {
                            if(_cameraInPhotoMode && !_photoIdle && _cameraElapsedMode) {
                                _camera.stopTakePhoto()
                            } else {
                                _camera.takePhoto()
                            }
                        }
                    } else {
                        if (_videoManager.recording) {
                            _videoManager.stopRecording()
                        } else {
                            _videoManager.startRecording()
                        }
                    }
                }
            }
        }

        //-- Status Information
        ColumnLayout {
            Layout.alignment:   Qt.AlignHCenter
            spacing:            0

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               _cameraName
                visible:            _cameraName !== ""
            }
            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               (_cameraInVideoMode && _camera.videoStatus === QGCCameraControl.VIDEO_CAPTURE_STATUS_RUNNING) ? _camera.recordTimeStr : "00:00:00"
                font.pointSize:     ScreenTools.largeFontPointSize
                visible:            _cameraInVideoMode
            }
            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               _activeVehicle && _cameraInPhotoMode ? ('00000' + _activeVehicle.cameraTriggerPoints.count).slice(-5) : "0000_cameraPhotoMode0"
                font.pointSize:     ScreenTools.largeFontPointSize
                visible:            _cameraInPhotoMode
            }
            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               _camera ? qsTr("Free Space: ") + _camera.storageFreeStr : ""
                font.pointSize:     ScreenTools.defaultFontPointSize
                visible:            _storageReady
            }
            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               _camera ? qsTr("Battery: ") + _camera.batteryRemainingStr : ""
                font.pointSize:     ScreenTools.defaultFontPointSize
                visible:            _batteryReady
            }
        }
    }

    Component {
        id: settingsDialogComponent

        QGCPopupDialog {
            title:      qsTr("Settings")
            buttons:    StandardButton.Close

            ColumnLayout {
                spacing: _margins

                GridLayout {
                    id:     gridLayout
                    flow:   GridLayout.TopToBottom
                    rows:   dynamicRows + (_camera ? _camera.activeSettings.length : 0)

                    property int dynamicRows: 10

                    // First column
                    QGCLabel {
                        text:               qsTr("Camera")
                        visible:            _multipleCameras
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Stream")
                        visible:            _multipleMavlinkCameraStreams
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Thermal View Mode")
                        visible:            _hasThermalVideoStream
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Blend Opacity")
                        visible:            _hasThermalVideoStream && _camera.thermalMode === QGCCameraControl.THERMAL_BLEND
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    // Mavlink Camera Protocol active settings
                    Repeater {
                        model: _camera ? _camera.activeSettings : []

                        QGCLabel {
                            text: _camera.getFact(modelData).shortDescription
                        }
                    }

                    QGCLabel {
                        text:               qsTr("Photo Mode")
                        visible:            _hasModes
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Photo Interval (seconds)")
                        width:              _labelFieldWidth
                        visible:            _cameraInPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Grid Lines")
                        visible:            _anyVideoStreamAvailable
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Video Screen Fit")
                        visible:            _anyVideoStreamAvailable
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Reset Camera Defaults")
                        visible:            _camera
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    QGCLabel {
                        text:               qsTr("Storage")
                        visible:            _storageSupported
                        onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                    }

                    // Second column
                    QGCComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              _cameraManager ? _cameraManager.cameraLabels : []
                        currentIndex:       _curCameraIndex
                        visible:            _multipleCameras
                        onActivated:        _cameraManager.currentCamera = index
                    }

                    QGCComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              _camera ? _camera.streamLabels : []
                        currentIndex:       _curStreamIndex
                        visible:            _multipleMavlinkCameraStreams
                        onActivated:        _camera.currentStream = index
                    }

                    QGCComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              [ qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture") ]
                        currentIndex:       _camera ? _camera.thermalMode : -1
                        visible:            _hasThermalVideoStream
                        onActivated:        _camera.thermalMode = index
                    }

                    QGCSlider {
                        Layout.fillWidth:           true
                        maximumValue:               100
                        minimumValue:               0
                        value:                      _camera ? _camera.thermalOpacity : 0
                        updateValueWhileDragging:   true
                        visible:                    _hasThermalVideoStream && _camera.thermalMode === QGCCameraControl.THERMAL_BLEND
                        onValueChanged:             _camera.thermalOpacity = value
                    }

                    // Mavlink Camera Protocol active settings
                    Repeater {
                        model: _camera ? _camera.activeSettings : []

                        RowLayout {
                            Layout.fillWidth:   true
                            spacing:            ScreenTools.defaultFontPixelWidth

                            property var    _fact:      _camera.getFact(modelData)
                            property bool   _isBool:    _fact.typeIsBool
                            property bool   _isCombo:   !_isBool && _fact.enumStrings.length > 0
                            property bool   _isSlider:  _fact && !isNaN(_fact.increment)
                            property bool   _isEdit:    !_isBool && !_isSlider && _fact.enumStrings.length < 1

                            FactComboBox {
                                Layout.fillWidth:   true
                                sizeToContents:     true
                                fact:               parent._fact
                                indexModel:         false
                                visible:            parent._isCombo
                            }
                            FactTextField {
                                Layout.fillWidth:   true
                                fact:               parent._fact
                                visible:            parent._isEdit
                            }
                            QGCSlider {
                                Layout.fillWidth:           true
                                maximumValue:               parent._fact.max
                                minimumValue:               parent._fact.min
                                stepSize:                   parent._fact.increment
                                visible:                    parent._isSlider
                                updateValueWhileDragging:   false
                                onValueChanged:             parent._fact.value = value
                                Component.onCompleted:      value = parent._fact.value
                            }
                            QGCSwitch {
                                checked:        parent._fact ? parent._fact.value : false
                                visible:        parent._isBool
                                onClicked:      parent._fact.value = checked ? 1 : 0
                            }
                        }
                    }

                    QGCComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        model:              [ qsTr("Single"), qsTr("Time Lapse") ]
                        currentIndex:       _camera ? _camera.photoMode : 0
                        visible:            _hasModes
                        onActivated:        _camera.photoMode = index
                    }

                    QGCSlider {
                        Layout.fillWidth:           true
                        maximumValue:               60
                        minimumValue:               1
                        stepSize:                   1
                        value:                      _camera ? _camera.photoLapse : 5
                        displayValue:               true
                        updateValueWhileDragging:   true
                        visible:                    _cameraInPhotoMode && _camera.photoMode === QGCCameraControl.PHOTO_CAPTURE_TIMELAPSE
                        onValueChanged: {
                            if (_camera) {
                                _camera.photoLapse = value
                            }
                        }
                    }

                    QGCSwitch {
                        checked:            _videoSettings.gridLines.rawValue
                        visible:            _anyVideoStreamAvailable
                        onClicked:          _videoSettings.gridLines.rawValue = checked ? 1 : 0
                    }

                    FactComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        fact:               _videoSettings.videoFit
                        indexModel:         false
                        visible:            _anyVideoStreamAvailable
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Reset")
                        visible:            _camera
                        onClicked:          resetPrompt.open()
                        MessageDialog {
                            id:                 resetPrompt
                            title:              qsTr("Reset Camera to Factory Settings")
                            text:               qsTr("Confirm resetting all settings?")
                            standardButtons:    StandardButton.Yes | StandardButton.No
                            onNo: resetPrompt.close()
                            onYes: {
                                _camera.resetSettings()
                                resetPrompt.close()
                            }
                        }
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Format")
                        visible:            _storageSupported
                        onClicked:          formatPrompt.open()
                        MessageDialog {
                            id:                 formatPrompt
                            title:              qsTr("Format Camera Storage")
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
            }
        }
    }
}
