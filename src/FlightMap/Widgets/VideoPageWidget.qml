/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.11
import QtPositioning            5.2
import QtQuick.Layouts          1.2
import QtQuick.Controls         2.4
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

/// Video streaming page for Instrument Panel PageView
Item {
    width:              pageWidth
    height:             videoGrid.y + videoGrid.height + _margins
    anchors.margins:    ScreenTools.defaultFontPixelWidth * 2
    anchors.centerIn:   parent

    property bool   _communicationLost:     activeVehicle ? activeVehicle.connectionLost : false
    property bool   _recordingVideo:        QGroundControl.videoManager.recording
    property bool   _decodingVideo:         QGroundControl.videoManager.decoding
    property bool   _streamingEnabled:      QGroundControl.settingsManager.videoSettings.streamConfigured
    property var    _dynamicCameras:        activeVehicle ? activeVehicle.dynamicCameras : null
    property int    _curCameraIndex:        _dynamicCameras ? _dynamicCameras.currentCamera : 0
    property bool   _isCamera:              _dynamicCameras ? _dynamicCameras.cameras.count > 0 : false
    property var    _camera:                _isCamera ? (_dynamicCameras.cameras.get(_curCameraIndex) && _dynamicCameras.cameras.get(_curCameraIndex).paramComplete ? _dynamicCameras.cameras.get(_curCameraIndex) : null) : null
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    GridLayout {
        id:                 videoGrid
        anchors.margins:    _margins
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        columns:            2
        columnSpacing:      _margins
        rowSpacing:         ScreenTools.defaultFontPixelHeight
        Connections {
            // For some reason, the normal signal is not reflected in the control below
            target: QGroundControl.settingsManager.videoSettings.streamEnabled
            onRawValueChanged: {
                enableSwitch.checked = QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue
            }
        }
        // Enable/Disable Video Streaming
        QGCLabel {
           text:                qsTr("Enable")
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
                    QGroundControl.videoManager.startVideo()
                } else {
                    QGroundControl.settingsManager.videoSettings.streamEnabled.rawValue = 0
                    QGroundControl.videoManager.stopVideo()
                }
            }
        }
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
            text:               qsTr("Video Fit")
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
            text:               qsTr("File Name");
            font.pointSize:     ScreenTools.smallFontPointSize
            visible:            QGroundControl.videoManager.isGStreamer
        }
        QGCTextField {
            id:                 videoFileName
            Layout.fillWidth:   true
            visible:            QGroundControl.videoManager.isGStreamer
        }
        //-- Video Recording
        QGCLabel {
           text:            _recordingVideo ? qsTr("Stop Recording") : qsTr("Record Stream")
           font.pointSize:  ScreenTools.smallFontPointSize
           visible:         QGroundControl.videoManager.isGStreamer
        }
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
                color:              (_decodingVideo && _streamingEnabled) ? "red" : "gray"
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
                enabled:        _decodingVideo && _streamingEnabled
                onClicked: {
                    if (_recordingVideo) {
                        QGroundControl.videoManager.stopRecording()
                        // reset blinking animation
                        recordBtnBackground.opacity = 1
                    } else {
                        QGroundControl.videoManager.startRecording(videoFileName.text)
                    }
                }
            }
        }
        QGCLabel {
            text:               qsTr("Video Streaming Not Configured")
            font.pointSize:     ScreenTools.smallFontPointSize
            visible:            !_streamingEnabled
            Layout.columnSpan:  2
        }
    }
}
