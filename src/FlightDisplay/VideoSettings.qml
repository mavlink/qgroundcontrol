/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.12
import QtQuick.Controls         2.12
import QtQuick.Layouts          1.12

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0

Popup {
    property QtObject settings: QGroundControl.settingsManager.videoSettings
    property string _videoSource:               settings.videoSource.value
    property bool   _isGst:                     QGroundControl.videoManager.isGStreamer
    property bool   _isUDP264:                  _isGst && _videoSource === settings.udp264VideoSource
    property bool   _isUDP265:                  _isGst && _videoSource === settings.udp265VideoSource
    property bool   _isRTSP:                    _isGst && _videoSource === settings.rtspVideoSource
    property bool   _isTCP:                     _isGst && _videoSource === settings.tcpVideoSource
    property bool   _isMPEGTS:                  _isGst && _videoSource === settings.mpegtsVideoSource

    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    modal: true
    focus: true
    background: Rectangle {
        color: qgcPal.windowShade
    }

    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    contentItem: GridLayout {
        id:                         videoGrid
        anchors.margins:            _margins
        columns:                    2

        QGCLabel {
            id:         videoSectionLabel
            text:       qsTr("Video")
            visible:    settings.visible && !QGroundControl.videoManager.autoStreamConfigured
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter
        }

        QGCLabel {
            text:                   qsTr("Video Source")
            visible:                settings.videoSource.visible
        }
        FactComboBox {
            id:                     videoSource
            indexModel:             false
            fact:                   settings.videoSource
            visible:                settings.videoSource.visible
        }

        QGCLabel {
            text:                   qsTr("UDP Port")
            visible:                (_isUDP264 || _isUDP265 || _isMPEGTS)  && settings.udpPort.visible
        }
        FactTextField {
            fact:                   settings.udpPort
            visible:                (_isUDP264 || _isUDP265 || _isMPEGTS) && settings.udpPort.visible
        }

        QGCLabel {
            text:                   qsTr("RTSP URL")
            visible:                _isRTSP && settings.rtspUrl.visible
        }
        FactTextField {
            fact:                   settings.rtspUrl
            visible:                _isRTSP && settings.rtspUrl.visible
        }

        QGCLabel {
            text:                   qsTr("TCP URL")
            visible:                _isTCP && settings.tcpUrl.visible
        }
        FactTextField {
            fact:                   settings.tcpUrl
            visible:                _isTCP && settings.tcpUrl.visible
        }
        QGCLabel {
            text:                   qsTr("Aspect Ratio")
            visible:                _isGst && settings.aspectRatio.visible
        }
        FactTextField {
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
            text:                   qsTr("Low Latency Mode")
            visible:                _isGst && settings.lowLatencyMode.visible
        }
        FactCheckBox {
            text:                   ""
            fact:                   settings.lowLatencyMode
            visible:                _isGst && settings.lowLatencyMode.visible
        }

        QGCLabel {
            id:                             videoRecSectionLabel
            text:                           qsTr("Video Recording")
            visible:                        (settings.visible && _isGst) || QGroundControl.videoManager.autoStreamConfigured
            Layout.columnSpan: 2
            Layout.alignment: Qt.AlignHCenter
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
            fact:                   settings.maxVideoSize
            visible:                settings.maxVideoSize.visible && settings.enableStorageLimit.value
        }

        QGCLabel {
            text:                   qsTr("Video File Format")
            visible:                settings.recordingFormat.visible
        }
        FactComboBox {
            fact:                   settings.recordingFormat
            visible:                settings.recordingFormat.visible
        }
    }
}
