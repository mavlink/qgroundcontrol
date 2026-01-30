import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VehicleSetup
import QGroundControl.FactControls

ColumnLayout {
    spacing: _margins

    required property var joystick

    readonly property var _joystickSettings: joystick.settings
    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: _margins
        Layout.rightMargin: _margins
        spacing: _margins

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("Center stick is zero throttle")
            fact: _joystickSettings.throttleModeCenterZero
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("Spring loaded throttle smoothing")
            fact: _joystickSettings.throttleSmoothing
            visible: fact.visible && _joystickSettings.throttleModeCenterZero.rawValue
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: fact.shortDescription
            fact: _joystickSettings.exponentialPct
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("Negative Thrust")
            fact: _joystickSettings.negativeThrust
            visible: globals.activeVehicle.supportsNegativeThrust && fact.visible
        }

        QGCCheckBoxSlider {
            id: advancedSettingsCheckbox
            Layout.fillWidth: true
            text: qsTr("Advanced Settings")
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("Advanced Settings")
        visible: advancedSettingsCheckbox.checked

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("Circle Correction")
            fact: _joystickSettings.circleCorrection
            visible: fact.visible
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: fact.shortDescription
            fact: _joystickSettings.axisFrequencyHz
            visible: fact.visible
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: fact.shortDescription
            fact: _joystickSettings.buttonFrequencyHz
            visible: fact.visible
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width
            spacing: 0
            visible: advancedSettingsCheckbox.checked

            FactCheckBoxSlider {
                text: qsTr("Deadband")
                fact: _joystickSettings.useDeadband
                visible: fact.visible
            }

            QGCLabel{
                Layout.fillWidth: true
                font.pointSize: ScreenTools.smallFontPointSize
                wrapMode: Text.WordWrap
                text: qsTr("Deadband can be set during the first step of calibration by gently wiggling each axis. ")
            }
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Pitch Extension")
            fact: _joystickSettings.enableManualControlPitchExtension
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Roll Extension")
            fact: _joystickSettings.enableManualControlRollExtension
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux1")
            fact: _joystickSettings.enableManualControlAux1
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux2")
            fact: _joystickSettings.enableManualControlAux2
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux3")
            fact: _joystickSettings.enableManualControlAux3
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux4")
            fact: _joystickSettings.enableManualControlAux4
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux5")
            fact: _joystickSettings.enableManualControlAux5
            visible: fact.visible
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: qsTr("MANUAL_CONTROL Aux6")
            fact: _joystickSettings.enableManualControlAux6
            visible: fact.visible
        }
    }
}
