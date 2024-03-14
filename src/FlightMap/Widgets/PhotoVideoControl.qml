/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtPositioning
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Vehicle
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls

Rectangle {
    width:      mainLayout.width + (_margins * 2)
    height:     mainLayout.height + (_margins * 2)
    color:      Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius:     _margins
    visible:    _camera.capturesVideo || _camera.capturesPhotos

    property real   _margins:                   ScreenTools.defaultFontPixelHeight / 2
    property real   _smallMargins:              ScreenTools.defaultFontPixelWidth / 2
    property var    _activeVehicle:             globals.activeVehicle
    property var    _cameraManager:             _activeVehicle.cameraManager
    property var    _camera:                    _cameraManager.currentCameraInstance
    property bool   _cameraInPhotoMode:         _camera.cameraMode === MavlinkCameraControl.CAM_MODE_PHOTO
    property bool   _cameraInVideoMode:         !_cameraInPhotoMode
    property bool   _videoCaptureIdle:          _camera.videoCaptureStatus === MavlinkCameraControl.VIDEO_CAPTURE_STATUS_STOPPED
    property bool   _photoCaptureSingleIdle:    _camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_IDLE
    property bool   _photoCaptureIntervalIdle:  _camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_INTERVAL_IDLE
    property bool   _photoCaptureIdle:          _photoCaptureSingleIdle || _photoCaptureIntervalIdle

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    DeadMouseArea { anchors.fill: parent }

    RowLayout {
        id:                 mainLayout
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        spacing:            _margins

        ColumnLayout {
            Layout.fillHeight:  true
            spacing:            _margins
            visible:            _camera.hasZoom

            QGCLabel {
                Layout.alignment:   Qt.AlignHCenter
                text:               qsTr("Zoom")
                font.pointSize:     ScreenTools.smallFontPointSize
            }

            QGCSlider {
                Layout.alignment:   Qt.AlignHCenter
                Layout.fillHeight:  true
                orientation:        Qt.Vertical
                to:                 100
                from:               0
                value:              _camera.zoomLevel
                live:               true
                onValueChanged:     _camera.zoomLevel = value
            }
        }
        
        ColumnLayout {
            spacing: _margins * 2

            ColumnLayout {
                spacing: _margins

                // Camera name
                QGCLabel {
                    Layout.alignment:   Qt.AlignHCenter
                    text:               _camera.modelName
                    visible:            _cameraManager.cameras.length > 1
                }

                // Photo/Video Mode Selector
                Rectangle {
                    Layout.alignment:   Qt.AlignHCenter
                    width:              ScreenTools.defaultFontPixelWidth * 10
                    height:             width / 2
                    color:              qgcPal.windowShadeLight
                    radius:             height * 0.5
                    visible:            _camera.hasModes

                    //-- Video Mode
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.height
                        height:                 parent.height
                        color:                  _cameraInVideoMode ? qgcPal.window : qgcPal.windowShadeLight
                        radius:                 height * 0.5
                        anchors.left:           parent.left
                        border.color:           qgcPal.text
                        border.width:           _cameraInPhotoMode ? 0 : 1

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
                                enabled:        _cameraInPhotoMode ? _photoCaptureIdle : true
                                onClicked:      _camera.setCameraModeVideo()
                            }
                        }
                    }
                    
                    //-- Photo Mode
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width:                  parent.height
                        height:                 parent.height
                        color:                  _cameraInPhotoMode ? qgcPal.window : qgcPal.windowShadeLight
                        radius:                 height * 0.5
                        anchors.right:          parent.right
                        border.color:           qgcPal.text
                        border.width:           _cameraInPhotoMode ? 1 : 0

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
                                enabled:        _cameraInVideoMode ? _videoCaptureIdle : true
                                onClicked:      _camera.setCameraModePhoto()
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
                    border.color:       qgcPal.buttonText
                    border.width:       3

                    Rectangle {
                        anchors.centerIn:   parent
                        width:              parent.width * (_isShootingInCurrentMode ? 0.5 : 0.75)
                        height:             width
                        radius:             _isShootingInCurrentMode ? 0 : width * 0.5
                        color:              _isShootingInCurrentMode || _canShootInCurrentMode ? qgcPal.colorRed : qgcPal.colorGrey

                        property bool _isShootingInPhotoMode:   _cameraInPhotoMode && _camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_IN_PROGRESS
                        property bool _isShootingInVideoMode:   (!_cameraInPhotoMode && _camera.videoCaptureStatus === MavlinkCameraControl.VIDEO_CAPTURE_STATUS_RUNNING)
                        property bool _isShootingInCurrentMode: _cameraInPhotoMode ? _isShootingInPhotoMode : _isShootingInVideoMode
                        property bool _isShootingInOtherMode:   _cameraInPhotoMode ? _isShootingInVideoMode : _isShootingInPhotoMode
                        property bool _canShootInCurrentMode:   _isShootingInOtherMode ? 
                                                                    (_cameraInPhotoMode ? _camera.photosInVideoMode : _camera.videoInPhotoMode) :
                                                                    true
                    }

                    MouseArea {
                        anchors.fill:   parent
                        onClicked:      toggleShooting()

                        function toggleShooting() {
                            if (_cameraInPhotoMode) {
                                if (_camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_INTERVAL_IN_PROGRESS) {
                                    _camera.stopTakePhoto()
                                } else if (_camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_IDLE || _camera.photoCaptureStatus === MavlinkCameraControl.PHOTO_CAPTURE_INTERVAL_IDLE) {
                                    _camera.takePhoto()
                                }
                            } else {
                                _camera.toggleVideoRecording()
                            }
                        }
                    }
                }

                // Record time / Capture count
                Rectangle {
                    Layout.alignment:       Qt.AlignHCenter
                    color:                  !_videoCaptureIdle && !_photoCaptureIdle ? "transparent" : qgcPal.colorRed
                    Layout.preferredWidth:  (_cameraInVideoMode ? videoRecordTime.width : photoCaptureCount.width) + (_smallMargins * 2)
                    Layout.preferredHeight: (_cameraInVideoMode ? videoRecordTime.height : photoCaptureCount.height)
                    radius:                 _margins / 2

                    // Video record time
                    QGCLabel {
                        id:                 videoRecordTime
                        anchors.leftMargin: _smallMargins
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        text:               _videoCaptureIdle ? "00:00:00" : _camera.recordTimeStr
                        font.pointSize:     ScreenTools.largeFontPointSize
                        visible:            _cameraInVideoMode
                    }

                    // Photo capture count
                    QGCLabel {
                        id:                 photoCaptureCount
                        anchors.leftMargin: _smallMargins
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        text:               _activeVehicle ? ('00000' + _activeVehicle.cameraTriggerPoints.count).slice(-5) : "00000"
                        font.pointSize:     ScreenTools.largeFontPointSize
                        visible:            _cameraInPhotoMode
                    }
                }

                //-- Status Information
                ColumnLayout {
                    Layout.alignment:   Qt.AlignHCenter
                    spacing:            0

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Free Space: ") + _camera.storageFreeStr
                        font.pointSize:     ScreenTools.defaultFontPointSize
                        visible:            _camera.storageStatus === MavlinkCameraControl.STORAGE_READY
                    }

                    QGCLabel {
                        Layout.alignment:   Qt.AlignHCenter
                        text:               qsTr("Battery: ") + _camera.batteryRemainingStr
                        font.pointSize:     ScreenTools.defaultFontPointSize
                        visible:            _camera.batteryRemaining >= 0
                    }
                }
            }

            ColumnLayout {
                id:                 trackingControls
                Layout.alignment:   Qt.AlignHCenter
                spacing:            _margins
                visible:            _camera && _camera.hasTracking

                Rectangle {
                    Layout.alignment:       Qt.AlignHCenter
                    color:                  _camera.trackingEnabled ? qgcPal.colorRed : qgcPal.windowShadeLight
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 6
                    Layout.preferredHeight: Layout.preferredWidth
                    border.color:           qgcPal.buttonText
                    border.width:           3
                    
                    QGCColoredImage {
                        height:             parent.height * 0.5
                        width:              height
                        anchors.centerIn:   parent
                        source:             "/qmlimages/TrackingIcon.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        color:              qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                _camera.trackingEnabled = !_camera.trackingEnabled;
                                if (!_camera.trackingEnabled) {
                                    !camera.stopTracking()
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    Layout.alignment:   Qt.AlignHCenter
                    text:               qsTr("Camera Tracking")
                    font.pointSize:     ScreenTools.defaultFontPointSize
                    visible:            _camera && _camera.hasTracking
                }
            }

            QGCColoredImage {
                Layout.alignment:       Qt.AlignHCenter
                source:                 "/res/gear-black.svg"
                mipmap:                 true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.preferredWidth:  Layout.preferredHeight
                sourceSize.height:      Layout.preferredHeight
                color:                  qgcPal.text
                fillMode:               Image.PreserveAspectFit

                QGCMouseArea {
                    fillItem:   parent
                    onClicked:  settingsDialogComponent.createObject(mainWindow).open()
                }
            }
        }

        Component {
            id: settingsDialogComponent

            QGCPopupDialog {
                title:      qsTr("Settings")
                buttons:    Dialog.Close

                property bool _multipleMavlinkCameras:          _cameraManager.cameras.count > 1
                property bool _multipleMavlinkCameraStreams:    _camera.streamLabels.length > 1
                property bool _cameraStorageSupported:          _camera.storageStatus !== MavlinkCameraControl.STORAGE_NOT_SUPPORTED
                property var  _videoSettings:                   QGroundControl.settingsManager.videoSettings

                ColumnLayout {
                    spacing: _margins

                    GridLayout {
                        id:     gridLayout
                        flow:   GridLayout.TopToBottom
                        rows:   dynamicRows + _camera.activeSettings.length

                        property int dynamicRows: 10

                        // First column
                        QGCLabel {
                            text:               qsTr("Camera")
                            visible:            _multipleMavlinkCameras
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Video Stream")
                            visible:            _multipleMavlinkCameraStreams
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Thermal View Mode")
                            visible:            _camera.thermalStreamInstance
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Blend Opacity")
                            visible:            _camera.thermalStreamInstance && _camera.thermalMode === MavlinkCameraControl.THERMAL_BLEND
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        // Mavlink Camera Protocol active settings
                        Repeater {
                            model: _camera.activeSettings

                            QGCLabel {
                                text: _camera.getFact(modelData).shortDescription
                            }
                        }

                        QGCLabel {
                            text:               qsTr("Photo Mode")
                            visible:            _camera.capturesPhotos
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Photo Interval (seconds)")
                            visible:            _camera.capturesPhotos && _camera.photoCaptureMode === MavlinkCameraControl.PHOTO_CAPTURE_TIMELAPSE
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Video Grid Lines")
                            visible:            _camera.hasVideoStream
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Video Screen Fit")
                            visible:            _camera.hasVideoStream
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Reset Camera Defaults")
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        QGCLabel {
                            text:               qsTr("Storage")
                            visible:            _cameraStorageSupported
                            onVisibleChanged:   gridLayout.dynamicRows += visible ? 1 : -1
                        }

                        // Second column
                        QGCComboBox {
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            model:              _cameraManager.cameraLabels
                            currentIndex:       _cameraManager.currentCamera
                            visible:            _multipleMavlinkCameras
                            onActivated:        (index) => { _cameraManager.currentCamera = index }
                        }

                        QGCComboBox {
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            model:              _camera.streamLabels
                            currentIndex:       _camera.currentStream
                            visible:            _multipleMavlinkCameraStreams
                            onActivated:        (index) => { _camera.currentStream = index }
                        }

                        QGCComboBox {
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            model:              [ qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture") ]
                            currentIndex:       _camera.thermalMode
                            visible:            _camera.thermalStreamInstance
                            onActivated:        (index) => { _camera.thermalMode = index }
                        }

                        QGCSlider {
                            Layout.fillWidth:   true
                            to:                 100
                            from:               0
                            value:              _camera.thermalOpacity
                            live:               true
                            visible:            _camera.thermalStreamInstance && _camera.thermalMode === MavlinkCameraControl.THERMAL_BLEND
                            onValueChanged:     _camera.thermalOpacity = value
                        }

                        // Mavlink Camera Protocol active settings
                        Repeater {
                            model: _camera.activeSettings

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
                                    to:               parent._fact.max
                                    from:               parent._fact.min
                                    stepSize:                   parent._fact.increment
                                    visible:                    parent._isSlider
                                    live:   false
                                    property bool initialized:  false

                                    onValueChanged: {
                                        if (!initialized) {
                                            return
                                        }
                                        parent._fact.value = value
                                    }

                                    Component.onCompleted: {
                                        value = parent._fact.value
                                        initialized = true
                                    }
                                }
                                QGCSwitch {
                                    checked:    parent._fact ? parent._fact.value : false
                                    visible:    parent._isBool
                                    onClicked:  parent._fact.value = checked ? 1 : 0
                                }
                            }
                        }

                        QGCComboBox {
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            model:              [ qsTr("Single"), qsTr("Time Lapse") ]
                            currentIndex:       _camera.photoCaptureMode
                            visible:            _camera.capturesPhotos
                            onActivated:        (index) => { _camera.photoCaptureMode = index }
                        }

                        QGCSlider {
                            Layout.fillWidth:   true
                            to:                 60
                            from:               1
                            stepSize:           1
                            value:              _camera.photoLapse
                            displayValue:       true
                            live:               true
                            visible:            _camera.capturesPhotos && _camera.photoCaptureMode === MavlinkCameraControl.PHOTO_CAPTURE_TIMELAPSE
                            onValueChanged:     _camera.photoLapse = value
                        }

                        QGCSwitch {
                            checked:    _videoSettings.gridLines.rawValue
                            visible:    _camera.hasVideoStream
                            onClicked:  _videoSettings.gridLines.rawValue = checked ? 1 : 0
                        }

                        FactComboBox {
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            fact:               _videoSettings.videoFit
                            indexModel:         false
                            visible:            _camera.hasVideoStream
                        }

                        QGCButton {
                            Layout.fillWidth:   true
                            text:               qsTr("Reset")
                            onClicked:          resetPrompt.open()
                            MessageDialog {
                                id:                 resetPrompt
                                title:              qsTr("Reset Camera to Factory Settings")
                                text:               qsTr("Confirm resetting all settings?")
                                buttons:            MessageDialog.Yes | MessageDialog.No

                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _camera.resetSettings()
                                        resetPrompt.close()
                                        break;
                                    case MessageDialog.No:
                                        resetPrompt.close()
                                        break;
                                    }
                                }
                            }
                        }

                        QGCButton {
                            Layout.fillWidth:   true
                            text:               qsTr("Format")
                            visible:            _cameraStorageSupported
                            onClicked:          formatPrompt.open()
                            MessageDialog {
                                id:                 formatPrompt
                                title:              qsTr("Format Camera Storage")
                                text:               qsTr("Confirm erasing all files?")
                                buttons:            MessageDialog.Yes | MessageDialog.No

                                onButtonClicked: function (button, role) {
                                    switch (button) {
                                    case MessageDialog.Yes:
                                        _camera.formatCard()
                                        formatPrompt.close()
                                        break;
                                    case MessageDialog.No:
                                        formatPrompt.close()
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
