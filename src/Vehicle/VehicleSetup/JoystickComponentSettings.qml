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

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("Center stick is zero throttle")
        checked: joystick.throttleModeCenterZero
        onClicked: joystick.throttleModeCenterZero = true
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("Spring loaded throttle smoothing")
        checked: joystick.throttleSmoothing
        enabled: joystick.throttleModeCenterZero
        onClicked: joystick.throttleSmoothing = checked
    }

    FactTextFieldSlider {
        Layout.fillWidth: true
        label: fact.shortDescription
        fact: joystick.exponentialPctFact
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("Allow negative Thrust")
        checked: joystick.negativeThrust
        visible: globals.activeVehicle.supportsNegativeThrust
        onClicked: joystick.negativeThrust = checked
    }

    QGCCheckBoxSlider {
        id: advancedSettingsCheckbox
        Layout.fillWidth: true
        text: qsTr("Advanced Settings")
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("Circle Correction")
        checked: joystick.circleCorrection
        onClicked: joystick.circleCorrection = checked
        visible: advancedSettingsCheckbox.checked
    }

    QGCCheckBoxSlider {
        Layout.fillWidth: true
        text: qsTr("MANUAL_CONTROL extensions (pitch/roll)")
        checked: joystick.enableManualControlExtensions
        onClicked: joystick.enableManualControlExtensions = checked
        visible: advancedSettingsCheckbox.checked
    }

    FactTextFieldSlider {
        Layout.fillWidth: true
        label: fact.shortDescription
        fact: joystick.axisFrequencyHzFact
        visible: advancedSettingsCheckbox.checked
    }

    FactTextFieldSlider {
        Layout.fillWidth: true
        label: fact.shortDescription
        fact: joystick.buttonFrequencyHzFact
        visible: advancedSettingsCheckbox.checked
    }

    ColumnLayout {
        Layout.preferredWidth: parent.width
        spacing: 0

        QGCCheckBox {
            text: qsTr("Use Deadband")
            checked: joystick.useDeadband
            onClicked: joystick.useDeadband = checked
            visible: advancedSettingsCheckbox.checked
        }

        QGCLabel{
            Layout.fillWidth: true
            font.pointSize: ScreenTools.smallFontPointSize
            wrapMode: Text.WordWrap
            visible: advancedSettingsCheckbox.checked
            text: qsTr("Deadband can be set during the first step of calibration by gently wiggling each axis. ")
        }
    }
}
