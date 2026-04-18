import QtQuick
import QtPositioning
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Rectangle {
    id: photoVideoControl
    width: mainLayout.width + (_smallMargins * 2)
    height: mainLayout.height + (_smallMargins * 2)
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.5)
    radius: _margins
    visible: _camera.capturesVideo || _camera.capturesPhotos || _camera.hasTracking || _camera.hasVideoStream

    property real _margins: ScreenTools.defaultFontPixelHeight / 2
    property real _smallMargins: ScreenTools.defaultFontPixelWidth / 2
    property var _activeVehicle: globals.activeVehicle
    property var _cameraManager: _activeVehicle.cameraManager
    property var _camera: _cameraManager.currentCameraInstance
    property bool _cameraInPhotoMode: _camera.cameraMode === MavlinkCameraControlInterface.CAM_MODE_PHOTO || _camera.cameraMode === MavlinkCameraControlInterface.CAM_MODE_SURVEY
    property bool _cameraInVideoMode: !_cameraInPhotoMode
    property bool _videoCaptureIdle: _camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateIdle
    property bool _photoCaptureIdle: _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateIdle

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    DeadMouseArea { anchors.fill: parent }

    RowLayout {
        id: mainLayout
        anchors.margins: _smallMargins
        anchors.top: parent.top
        anchors.left: parent.left
        spacing: _margins

        ColumnLayout {
            Layout.fillHeight: true
            spacing: 0
            visible: _camera.hasZoom

            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Zoom")
                font.pointSize: ScreenTools.smallFontPointSize
            }

            QGCSlider {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillHeight: true
                orientation: Qt.Vertical
                to: 100
                from: 0
                value: _camera.zoomLevel
                live: true
                onValueChanged: _camera.zoomLevel = value
            }
        }

        ColumnLayout {
            spacing: _margins

            // Camera name
            QGCLabel {
                Layout.alignment: Qt.AlignHCenter
                text: _camera.modelName
                visible: _cameraManager.cameras.length > 1
            }

            // Photo/Video Mode Selector
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: ScreenTools.defaultFontPixelWidth * 10
                height: width / 2
                color: qgcPal.windowShadeLight
                radius: height * 0.5
                visible: _camera.hasModes

                //-- Video Mode
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.height
                    height: parent.height
                    color: _cameraInVideoMode ? qgcPal.window : qgcPal.windowShadeLight
                    radius: height * 0.5
                    anchors.left: parent.left
                    border.color: qgcPal.text
                    border.width: _cameraInPhotoMode ? 0 : 1

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/camera_video.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: _cameraInVideoMode ? qgcPal.colorGreen : qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            enabled: _cameraInPhotoMode ? _photoCaptureIdle : true
                            onClicked: _camera.setCameraModeVideo()
                        }
                    }
                }

                //-- Photo Mode
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.height
                    height: parent.height
                    color: _cameraInPhotoMode ? qgcPal.window : qgcPal.windowShadeLight
                    radius: height * 0.5
                    anchors.right: parent.right
                    border.color: qgcPal.text
                    border.width: _cameraInPhotoMode ? 1 : 0

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/camera_photo.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: _cameraInPhotoMode ? qgcPal.colorGreen : qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            enabled: _cameraInVideoMode ? _videoCaptureIdle : true
                            onClicked: _camera.setCameraModePhoto()
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: _smallMargins

                // Start/Stop Video button
                Rectangle {
                    id: videoCaptureButton
                    Layout.alignment: Qt.AlignHCenter
                    color: videoCaptureButtonPalette.button
                    width: ScreenTools.defaultFontPixelWidth * 6
                    height: width
                    radius: width * 0.5
                    border.width: 1
                    border.color: videoCaptureButtonPalette.buttonBorder
                    visible: (_camera.hasModes && _cameraInVideoMode) || (!_camera.hasModes && _camera.capturesVideo)
                    enabled: _camera.captureVideoState !== MavlinkCameraControlInterface.CaptureVideoStateDisabled

                    QGCPalette { id: videoCaptureButtonPalette; colorGroupEnabled: videoCaptureButton.enabled }

                    Rectangle {
                        anchors.centerIn: parent
                        anchors.alignWhenCentered: false
                        color: videoCaptureButtonPalette.buttonBorder
                        width: parent.width * 0.75
                        height: width
                        radius: width * 0.5
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        anchors.alignWhenCentered: false
                        width: parent.width * (_isCapturing ? 0.5 : 0.75)
                        height: width
                        radius: _isCapturing ? ScreenTools.defaultFontPixelWidth * 0.5 : width * 0.5
                        color: videoCaptureButtonPalette.videoCaptureButtonColor
                        border.width: 1
                        border.color: videoCaptureButtonPalette.buttonBorder

                        property bool _isCapturing: _camera.captureVideoState === MavlinkCameraControlInterface.CaptureVideoStateCapturing
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: _camera.toggleVideoRecording()
                    }
                }

                QGCLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Video")
                    font.pointSize: ScreenTools.smallFontPointSize
                    visible: videoCaptureButton.visible && photoCaptureButton.visible
                }

                // Record time
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    color: _videoCaptureIdle ? "transparent" : videoCaptureButtonPalette.videoCaptureButtonColor
                    Layout.preferredWidth: videoRecordTime.width + (_smallMargins * 2)
                    Layout.preferredHeight: videoRecordTime.height
                    radius: _smallMargins
                    visible: videoCaptureButton.visible

                    QGCLabel {
                        id: videoRecordTime
                        anchors.leftMargin: _smallMargins
                        anchors.left: parent.left
                        anchors.top: parent.top
                        text: _videoCaptureIdle ? "00:00:00" : _camera.recordTimeStr
                    }
                }

                Item {
                    Layout.alignment: Qt.AlignHCenter
                    width: 1
                    height: 1
                    visible: videoCaptureButton.visible && photoCaptureButton.visible
                }

                // Take Photo button
                Rectangle {
                    id: photoCaptureButton
                    Layout.alignment: Qt.AlignHCenter
                    color: photoCaptureButtonPalette.button
                    width: ScreenTools.defaultFontPixelWidth * 6
                    height: width
                    radius: width * 0.5
                    border.width: 1
                    border.color: photoCaptureButtonPalette.buttonBorder
                    visible: (_camera.hasModes && _cameraInPhotoMode) || (!_camera.hasModes && (_camera.hasVideoStream || _camera.capturesPhotos))
                    enabled: _camera.capturePhotosState !== MavlinkCameraControlInterface.CapturePhotosStateDisabled

                    QGCPalette { id: photoCaptureButtonPalette; colorGroupEnabled: photoCaptureButton.enabled }

                    Rectangle {
                        anchors.centerIn: parent
                        anchors.alignWhenCentered: false
                        color: photoCaptureButtonPalette.buttonBorder
                        width: parent.width * 0.75
                        height: width
                        radius: width * 0.5
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        anchors.alignWhenCentered: false
                        width: parent.width * (_isCapturing ? 0.5 : 0.75)
                        height: width
                        radius: _isCapturing ? ScreenTools.defaultFontPixelWidth * 0.5 : width * 0.5
                        color: photoCaptureButtonPalette.photoCaptureButtonColor
                        border.width: 1
                        border.color: photoCaptureButtonPalette.buttonBorder

                        property bool _isCapturing: _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingSinglePhoto ||
                                                        _camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingMultiplePhotos
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (_camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateCapturingMultiplePhotos) {
                                _camera.stopTakePhoto()
                            } else if (_camera.capturePhotosState === MavlinkCameraControlInterface.CapturePhotosStateIdle) {
                                _camera.takePhoto()
                            }
                        }
                    }
                }

                QGCLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Photo")
                    font.pointSize: ScreenTools.smallFontPointSize
                    visible: videoCaptureButton.visible && photoCaptureButton.visible
                }

                // Capture count
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    color: _photoCaptureIdle ? "transparent" : photoCaptureButtonPalette.photoCaptureButtonColor
                    Layout.preferredWidth: photoCaptureCount.width + (_smallMargins * 2)
                    Layout.preferredHeight: photoCaptureCount.height
                    radius: _smallMargins
                    visible: photoCaptureButton.visible

                    QGCLabel {
                        id: photoCaptureCount
                        anchors.leftMargin: _smallMargins
                        anchors.left: parent.left
                        anchors.top: parent.top
                        text: _activeVehicle ? ('00000' + _activeVehicle.cameraTriggerPoints.count).slice(-5) : "00000"
                    }
                }
            }

            //-- Status Information
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                visible: storageStatus.visible || batteryStatus.visible

                QGCLabel {
                    id: storageStatus
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Free: ") + _camera.storageFreeStr
                    font.pointSize: ScreenTools.defaultFontPointSize
                    visible: _camera.storageStatus === MavlinkCameraControlInterface.STORAGE_READY
                }

                QGCLabel {
                    id: batteryStatus
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Battery: ") + _camera.batteryRemainingStr
                    font.pointSize: ScreenTools.defaultFontPointSize
                    visible: _camera.batteryRemaining >= 0
                }
            }

            ColumnLayout {
                id: trackingControls
                Layout.alignment: Qt.AlignHCenter
                spacing: 0
                visible: _camera.hasTracking

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    color: _camera.trackingEnabled ? qgcPal.colorRed : qgcPal.windowShadeLight
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 6
                    Layout.preferredHeight: Layout.preferredWidth
                    border.color: qgcPal.buttonText
                    border.width: 3

                    QGCColoredImage {
                        height: parent.height * 0.5
                        width: height
                        anchors.centerIn: parent
                        source: "/qmlimages/TrackingIcon.svg"
                        fillMode: Image.PreserveAspectFit
                        sourceSize.height: height
                        color: qgcPal.text

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                _camera.trackingEnabled = !_camera.trackingEnabled;
                                if (!_camera.trackingEnabled) {
                                    _camera.stopTracking()
                                }
                            }
                        }
                    }
                }

                QGCLabel {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Camera Tracking")
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }

            QGCColoredImage {
                Layout.alignment: Qt.AlignHCenter
                source: "/res/gear-black.svg"
                mipmap: true
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                Layout.preferredWidth: Layout.preferredHeight
                sourceSize.height: Layout.preferredHeight
                color: qgcPal.text
                fillMode: Image.PreserveAspectFit

                QGCMouseArea {
                    fillItem: parent
                    onClicked: settingsDialogFactory.open()
                }
            }
        }

        QGCPopupDialogFactory {
            id: settingsDialogFactory

            dialogComponent: Component {
                CameraSettingsDialog {
                    camera:        _camera
                    cameraManager: _cameraManager
                    contentSpacing: _margins
                }
            }
        }
    }
}
