import QtQuick          2.3
import QtQuick.Layouts  1.12
import QtQuick.Controls 2.12 as QQC2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0

Rectangle {
    property var settings: QGroundControl.settingsManager.videoSettings
    property string _videoSource: QGroundControl.settingsManager.videoSettings.videoSource.value
    property bool _isGst: QGroundControl.videoManager.isGStreamer
    property bool _isUDP264: _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.udp264VideoSource
    property bool _isUDP265: _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.udp265VideoSource
    property bool _isRTSP: _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.rtspVideoSource
    property bool _isTCP: _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.tcpVideoSource
    property bool _isMPEGTS: _isGst && _videoSource === QGroundControl.settingsManager.videoSettings.mpegtsVideoSource
    property real _comboFieldWidth: ScreenTools.defaultFontPixelWidth * 30

    // Video Popup.
    property bool   _communicationLost: activeVehicle ? activeVehicle.connectionLost : false
    property var    _videoReceiver: QGroundControl.videoManager.videoReceiver
    property bool   _recordingVideo: _videoReceiver && _videoReceiver.recording
    property bool   _videoRunning: _videoReceiver && _videoReceiver.videoRunning
    property bool   _streamingEnabled: QGroundControl.settingsManager.videoSettings.streamConfigured
    property var    _dynamicCameras: activeVehicle ? activeVehicle.dynamicCameras : null
    property int    _curCameraIndex: _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera: _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _currentCamera: _dynamicCameras.cameras.get(_curCameraIndex)
    property var    _camera: !_isCamera ? null :
                            _dynamicCameras.cameras.get(_curCameraIndex) && _dynamicCameras.cameras.get(_curCameraIndex).paramComplete
                            ? _dynamicCameras.cameras.get(_curCameraIndex) : null


    Layout.preferredWidth:  videoGrid.width + (_margins * 2)
    Layout.preferredHeight: videoGrid.height + (_margins * 2)
    Layout.fillWidth: true
    color: qgcPal.windowShade
    visible: videoSectionLabel.visible

    GridLayout {
        id:                         videoGrid
        anchors.margins:            _margins
        anchors.top:                parent.top
        anchors.horizontalCenter:   parent.horizontalCenter
        Layout.fillWidth:           false
        Layout.fillHeight:          false
        columns:                    2
        QGCLabel {
            text:                   qsTr("Video Source")
            visible:                settings.videoSource.visible
        }
        FactComboBox {
            id:                     videoSource
            Layout.preferredWidth:  _comboFieldWidth
            indexModel:             false
            fact:                   settings.videoSource
            visible:                settings.videoSource.visible
        }

        QGCLabel {
            text:                   qsTr("UDP Port")
            visible:                (_isUDP264 || _isUDP265 || _isMPEGTS)  && settings.udpPort.visible
        }
        FactTextField {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.udpPort
            visible:                (_isUDP264 || _isUDP265 || _isMPEGTS) && settings.udpPort.visible
        }

        QGCLabel {
            text:                   qsTr("RTSP URL")
            visible:                _isRTSP && settings.rtspUrl.visible
        }
        FactTextField {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.rtspUrl
            visible:                _isRTSP && settings.rtspUrl.visible
        }

        QGCLabel {
            text:                   qsTr("TCP URL")
            visible:                _isTCP && settings.tcpUrl.visible
        }
        FactTextField {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.tcpUrl
            visible:                _isTCP && settings.tcpUrl.visible
        }
        QGCLabel {
            text:                   qsTr("Aspect Ratio")
            visible:                _isGst && settings.aspectRatio.visible
        }
        FactTextField {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.aspectRatio
            visible:                _isGst && settings.aspectRatio.visible
        }

        QGCLabel {
            text:                   qsTr("Disable When Disarmed")
            visible:                _isGst && settings.disableWhenDisarmed.visible
        }
        FactCheckBox {
            text:                   ""
            fact:                   settings.disableWhenDisarmed
            visible:                _isGst && settings.disableWhenDisarmed.visible
        }

        QGCLabel {
            text:                   qsTr("Auto-Delete Files")
            visible:                settings.enableStorageLimit.visible
        }
        FactCheckBox {
            text:                   ""
            fact:                   settings.enableStorageLimit
            visible:                settings.enableStorageLimit.visible
        }

        QGCLabel {
            text:                   qsTr("Max Storage Usage")
            visible:                settings.maxVideoSize.visible && settings.enableStorageLimit.value
        }
        FactTextField {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.maxVideoSize
            visible:                settings.maxVideoSize.visible && settings.enableStorageLimit.value
        }

        QGCLabel {
            text:                   qsTr("Video File Format")
            visible:                settings.recordingFormat.visible
        }
        FactComboBox {
            Layout.preferredWidth:  _comboFieldWidth
            fact:                   settings.recordingFormat
            visible:                settings.recordingFormat.visible
        }
    }

    Connections {
        // For some reason, the normal signal is not reflected in the control below
        target: QGroundControl.settingsManager.videoSettings.streamEnabled
        onRawValueChanged: {
            enableSwitch.checked = QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue
        }
    }

    // TODO: Remove this.
    QGCLabel {
       text:                qsTr("Enable Stream")
       font.pointSize:      ScreenTools.smallFontPointSize
       visible:             !_camera || !_camera.autoStream
    }

    QGCSwitch {
        id:                 enableSwitch
        visible:            !_camera || !_camera.autoStream
        enabled:            _streamingEnabled
        checked:            QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue
        Layout.alignment:   Qt.AlignHCenter
        onClicked: {
            if(checked) {
                QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue = 1
                _videoReceiver.start()
            } else {
                QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue = 0
                _videoReceiver.stop()
            }
        }
    }

    // TODO: Remove this.
    // Grid Lines
    QGCLabel {
       text:                qsTr("Grid Lines")
       font.pointSize:      ScreenTools.smallFontPointSize
       visible:             QGroundControl.videoManager.isGStreamer && QGroundControl.settingsManager.videoSettings.gridLines.visible
    }
    QGCSwitch {
        enabled:            _streamingEnabled && activeVehicle
        checked:            QGroundControl.settingsManager.videoSettings.gridLines.rawValue
        visible:            QGroundControl.videoManager.isGStreamer && QGroundControl.settingsManager.videoSettings.gridLines.visible
        Layout.alignment:   Qt.AlignHCenter
        onClicked: {
            if(checked) {
                QGroundControl.settingsManager.videoSettings.gridLines.rawValue = 1
            } else {
                QGroundControl.settingsManager.videoSettings.gridLines.rawValue = 0
            }
        }
    }

    //-- Video Fit
    QGCLabel {
        text:               qsTr("Video Screen Fit")
        visible:            QGroundControl.videoManager.isGStreamer
        font.pointSize:     ScreenTools.smallFontPointSize
    }
    FactComboBox {
        fact:               QGroundControl.settingsManager.videoSettings.videoFit
        visible:            QGroundControl.videoManager.isGStreamer
        indexModel:         false
        Layout.alignment:   Qt.AlignHCenter
    }
    QGCLabel {
        text: qsTr("File Name");
        visible: QGroundControl.videoManager.isGStreamer
    }
    QQC2.TextField {
        id: videoFileName
        visible: QGroundControl.videoManager.isGStreamer
        width: 100
    }
    //-- Video Recording
    QGCLabel {
       text:            _recordingVideo ? qsTr("Stop Recording") : qsTr("Record Stream")
       font.pointSize:  ScreenTools.smallFontPointSize
       visible:         QGroundControl.videoManager.isGStreamer
    }

    // TODO: Remove this.
    // Button to start/stop video recording
    Item {
        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
        height:             ScreenTools.defaultFontPixelHeight * 2
        width:              height
        Layout.alignment:   Qt.AlignHCenter
        visible:            QGroundControl.videoManager.isGStreamer
        Rectangle {
            id:                 recordBtnBackground
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            width:              height
            radius:             _recordingVideo ? 0 : height
            color:              (_videoRunning && _streamingEnabled) ? "red" : "gray"
            SequentialAnimation on opacity {
                running:        _recordingVideo
                loops:          Animation.Infinite
                PropertyAnimation { to: 0.5; duration: 500 }
                PropertyAnimation { to: 1.0; duration: 500 }
            }
        }
        QGCColoredImage {
            anchors.top:                parent.top
            anchors.bottom:             parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            width:                      height * 0.625
            sourceSize.width:           width
            source:                     "/qmlimages/CameraIcon.svg"
            visible:                    recordBtnBackground.visible
            fillMode:                   Image.PreserveAspectFit
            color:                      "white"
        }
        MouseArea {
            anchors.fill:   parent
            enabled:        _videoRunning && _streamingEnabled
            onClicked: {
                if (_recordingVideo) {
                    _videoReceiver.stopRecording()
                    // reset blinking animation
                    recordBtnBackground.opacity = 1
                } else {
                    _videoReceiver.startRecording(videoFileName.text)
                }
            }
        }
    }
}
