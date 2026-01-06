import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.VehicleSetup
import QGroundControl.FactControls

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight / 2

    required property var joystick

    readonly property var _joystickSettings: joystick.settings

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

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("Circle Correction")
        fact: _joystickSettings.circleCorrection
        visible: advancedSettingsCheckbox.checked && fact.visible
    }

    FactCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("MANUAL_CONTROL Extensions (pitch/roll)")
        fact: _joystickSettings.enableManualControlExtensions
        visible: advancedSettingsCheckbox.checked && fact.visible
    }

    FactTextFieldSlider {
        Layout.fillWidth: true
        label: fact.shortDescription
        fact: _joystickSettings.axisFrequencyHz
        visible: advancedSettingsCheckbox.checked && fact.visible
    }

    FactTextFieldSlider {
        Layout.fillWidth: true
        label: fact.shortDescription
        fact: _joystickSettings.buttonFrequencyHz
        visible: advancedSettingsCheckbox.checked && fact.visible
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
}
