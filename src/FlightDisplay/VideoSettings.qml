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
}
