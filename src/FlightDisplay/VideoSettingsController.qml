/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0

RowLayout {
    QGCLabel {
        id:         videoSectionLabel
        text:       qsTr("Video")
        visible:    QGroundControl.settingsManager.videoSettings.visible && !QGroundControl.videoManager.autoStreamConfigured
    }
    Rectangle {
        Layout.preferredWidth:  videoGrid.width + (_margins * 2)
        Layout.preferredHeight: videoGrid.height + (_margins * 2)
        Layout.fillWidth:       true
        color:                  qgcPal.windowShade
        visible:                videoSectionLabel.visible

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
                visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
            }
            FactComboBox {
                id:                     videoSource
                indexModel:             false
                fact:                   QGroundControl.settingsManager.videoSettings.videoSource
                visible:                QGroundControl.settingsManager.videoSettings.videoSource.visible
            }

            QGCLabel {
                text:                   qsTr("UDP Port")
                visible:                QGroundControl.settingsManager.videoSettings.udpPort.visible
            }
            FactTextField {
                fact:                   QGroundControl.settingsManager.videoSettings.udpPort
                visible:                QGroundControl.settingsManager.videoSettings.udpPort.visible
            }

            QGCLabel {
                text:                   qsTr("RTSP URL")
                visible:                QGroundControl.settingsManager.videoSettings.rtspUrl.visible
            }
            FactTextField {
                fact:                   QGroundControl.settingsManager.videoSettings.rtspUrl
                visible:                QGroundControl.settingsManager.videoSettings.rtspUrl.visible
            }

            QGCLabel {
                text:                   qsTr("TCP URL")
                visible:                QGroundControl.settingsManager.videoSettings.tcpUrl.visible
            }
            FactTextField {
                fact:                   QGroundControl.settingsManager.videoSettings.tcpUrl
                visible:                QGroundControl.settingsManager.videoSettings.tcpUrl.visible
            }
            QGCLabel {
                text:                   qsTr("Aspect Ratio")
                visible:                QGroundControl.settingsManager.videoSettings.aspectRatio.visible
            }
            FactTextField {
                fact:                   QGroundControl.settingsManager.videoSettings.aspectRatio
                visible:                QGroundControl.settingsManager.videoSettings.aspectRatio.visible
            }

            QGCLabel {
                text:                   qsTr("Disable When Disarmed")
                visible:                QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
            }
            FactCheckBox {
                text:                   ""
                fact:                   QGroundControl.settingsManager.videoSettings.disableWhenDisarmed
                visible:                QGroundControl.settingsManager.videoSettings.disableWhenDisarmed.visible
            }
        }
    }

    Item { width: 1; height: _margins }

    QGCLabel {
        id:                             videoRecSectionLabel
        text:                           qsTr("Video Recording")
        visible:                        (QGroundControl.settingsManager.videoSettings.visible) || QGroundControl.videoManager.autoStreamConfigured
    }
    Rectangle {
        Layout.preferredWidth:          videoRecCol.width  + (_margins * 2)
        Layout.preferredHeight:         videoRecCol.height + (_margins * 2)
        Layout.fillWidth:               true
        color:                          qgcPal.windowShade
        visible:                        videoRecSectionLabel.visible

        GridLayout {
            id:                         videoRecCol
            anchors.margins:            _margins
            anchors.top:                parent.top
            anchors.horizontalCenter:   parent.horizontalCenter
            Layout.fillWidth:           false
            columns:                    2

            QGCLabel {
                text:                   qsTr("Auto-Delete Files")
                visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
            }
            FactCheckBox {
                text:                   ""
                fact:                   QGroundControl.settingsManager.videoSettings.enableStorageLimit
                visible:                QGroundControl.settingsManager.videoSettings.enableStorageLimit.visible
            }

            QGCLabel {
                text:                   qsTr("Max Storage Usage")
                visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
            }
            FactTextField {
                fact:                   QGroundControl.settingsManager.videoSettings.maxVideoSize
                visible:                QGroundControl.settingsManager.videoSettings.maxVideoSize.visible && QGroundControl.settingsManager.videoSettings.enableStorageLimit.value
            }

            QGCLabel {
                text:                   qsTr("Video File Format")
                visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
            }
            FactComboBox {
                fact:                   QGroundControl.settingsManager.videoSettings.recordingFormat
                visible:                QGroundControl.settingsManager.videoSettings.recordingFormat.visible
            }
        }
    }
}
