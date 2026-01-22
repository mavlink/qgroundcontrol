import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Joystick Indicator
Item {
    id:             control
    width:          joystickIcon.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator:    _activeJoystick
    property var  _activeJoystick:  joystickManager.activeJoystick

    QGCPalette { id: qgcPal }

    Component {
        id: joystickInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: _activeJoystick ? _activeJoystick.name : qsTr("Joystick")

                GridLayout {
                    columns:        2
                    columnSpacing:  ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel { text: qsTr("Enabled:") }
                    QGCLabel {
                        text: {
                            if (!globals.activeVehicle)
                                return qsTr("No Vehicle")
                            return globals.activeVehicle.joystickEnabled ? qsTr("Yes") : qsTr("No")
                        }
                        color: {
                            if (!globals.activeVehicle)
                                return qgcPal.buttonText
                            return globals.activeVehicle.joystickEnabled ? qgcPal.buttonText : "orange"
                        }
                    }

                    QGCLabel { text: qsTr("Type:") }
                    QGCLabel {
                        text: _activeJoystick ? (_activeJoystick.isGamepad ? _activeJoystick.gamepadType || qsTr("Gamepad") : qsTr("Joystick")) : ""
                    }

                    QGCLabel {
                        text:    qsTr("Connection:")
                        visible: _activeJoystick && _activeJoystick.connectionType
                    }
                    QGCLabel {
                        text:    _activeJoystick ? _activeJoystick.connectionType : ""
                        visible: _activeJoystick && _activeJoystick.connectionType
                    }

                    QGCLabel { text: qsTr("Inputs:") }
                    QGCLabel {
                        text: _activeJoystick ? qsTr("%1 axes, %2 buttons").arg(_activeJoystick.axisCount).arg(_activeJoystick.buttonCount) : ""
                    }

                    QGCLabel {
                        text:    qsTr("Battery:")
                        visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
                    }
                    QGCLabel {
                        text:    _activeJoystick && _activeJoystick.batteryPercent >= 0 ? qsTr("%1%").arg(_activeJoystick.batteryPercent) : ""
                        color:   _activeJoystick && _activeJoystick.batteryPercent < 20 ? "red" : qgcPal.buttonText
                        visible: _activeJoystick && _activeJoystick.batteryPercent >= 0
                    }

                    QGCLabel {
                        text:    qsTr("Features:")
                        visible: _activeJoystick && (_activeJoystick.hasRumble || _activeJoystick.hasLED)
                    }
                    QGCLabel {
                        property var features: {
                            var list = []
                            if (_activeJoystick) {
                                if (_activeJoystick.hasRumble) list.push(qsTr("Rumble"))
                                if (_activeJoystick.hasLED) list.push(qsTr("LED"))
                            }
                            return list.join(", ")
                        }
                        text:    features
                        visible: _activeJoystick && (_activeJoystick.hasRumble || _activeJoystick.hasLED)
                    }
                }
            }
        }
    }

    QGCColoredImage {
        id:                 joystickIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        sourceSize.height:  height
        source:             "/qmlimages/Joystick.png"
        fillMode:           Image.PreserveAspectFit
        color: {
            if (!globals.activeVehicle) {
                return qgcPal.buttonText
            }
            if (globals.activeVehicle.joystickEnabled) {
                return qgcPal.buttonText
            }
            return "orange"
        }
    }

    QGCMouseArea {
        fillItem:   joystickIcon
        onClicked:  mainWindow.showIndicatorDrawer(joystickInfoPage, control)
    }
}
