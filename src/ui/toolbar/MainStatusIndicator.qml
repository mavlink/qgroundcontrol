/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    id:                     _root
    Layout.preferredWidth:  mainStatusLabel.contentWidth + ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real   _margins:       ScreenTools.defaultFontPixelWidth

    Component {
        id: mainStatusInfo

        Rectangle {
            width:          mainLayout.width   + (_margins * 2)
            height:         mainLayout.height  + (_margins * 2)
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            color:          qgcPal.window
            border.color:   qgcPal.text

            GridLayout {
                id:                 mainLayout
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                rowSpacing:         ScreenTools.defaultFontPixelWidth / 2
                columnSpacing:      rowSpacing
                rows:               _activeVehicle.sysStatusSensorInfo.sensorNames.length
                flow:               GridLayout.TopToBottom

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorNames

                    QGCLabel {
                        text: modelData
                    }
                }

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorStatus

                    QGCLabel {
                        text: modelData
                    }
                }
            }
        }
    }

    QGCLabel {
        id:                     mainStatusLabel
        text:                   mainStatusText()
        font.pointSize:         ScreenTools.largeFontPointSize
        anchors.verticalCenter: parent.verticalCenter

        property string _commLostText:      qsTr("Communication Lost")
        property string _readyToFlyText:    qsTr("Ready To Fly")
        property string _notReadyToFlyText: qsTr("Not Ready")
        property string _disconnectedText:  qsTr("Disconnected")
        property string _armedText:         qsTr("Armed")
        property string _flyingText:        qsTr("Flying")
        property string _landingText:       qsTr("Landing")

        function mainStatusText() {
            var statusText
            if (_activeVehicle) {
                if (_communicationLost) {
                    _mainStatusBGColor = "red"
                    return mainStatusLabel._commLostText
                }
                if (_activeVehicle.armed) {
                    _mainStatusBGColor = "green"
                    if (_activeVehicle.flying) {
                        return mainStatusLabel._flyingText
                    } else if (_activeVehicle.landing) {
                        return mainStatusLabel._landingText
                    } else {
                        return mainStatusLabel._armedText
                    }
                } else {
                    if (_activeVehicle.readyToFlyAvailable) {
                        if (_activeVehicle.readyToFly) {
                            _mainStatusBGColor = "green"
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "yellow"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    } else {
                        // Best we can do is determine readiness based on AutoPilot component setup and health indicators from SYS_STATUS
                        if (_activeVehicle.allSensorsHealthy && _activeVehicle.autopilot.setupComplete) {
                            _mainStatusBGColor = "green"
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "yellow"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    }
                }
            } else {
                _mainStatusBGColor = qgcPal.brandingPurple
                return mainStatusLabel._disconnectedText
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        enabled:        _activeVehicle
        onClicked: {
            mainWindow.showPopUp(_root, mainStatusInfo)
        }
    }
}
