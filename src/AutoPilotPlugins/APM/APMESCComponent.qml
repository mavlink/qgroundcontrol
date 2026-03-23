import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id: escPage
    pageComponent: escPageComponent

    FactPanelController {
        id: controller
    }

    Component {
        id: escPageComponent

        ColumnLayout {
            width: availableWidth
            spacing: _margins

            // ESC Configuration properties - supports both MOT_* (Copter/Rover/Sub) and Q_M_* (QuadPlane) prefixes
            property bool _isQuadPlane: !controller.parameterExists(-1, "MOT_PWM_TYPE") && controller.parameterExists(-1, "Q_M_PWM_TYPE")
            property string _escPrefix: _isQuadPlane ? "Q_M_" : "MOT_"

            property bool _motPwmTypeAvailable: controller.parameterExists(-1, _escPrefix + "PWM_TYPE")
            property bool _motPwmMinAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MIN")
            property bool _motPwmMaxAvailable: controller.parameterExists(-1, _escPrefix + "PWM_MAX")
            property bool _motSpinArmAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_ARM")
            property bool _motSpinMinAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MIN")
            property bool _motSpinMaxAvailable: controller.parameterExists(-1, _escPrefix + "SPIN_MAX")
            property bool _servoDshotEscAvailable: controller.parameterExists(-1, "SERVO_DSHOT_ESC")
            property bool _servoDshotRateAvailable: controller.parameterExists(-1, "SERVO_DSHOT_RATE")

            property Fact _motPwmType: controller.getParameterFact(-1, _escPrefix + "PWM_TYPE", false /* reportMissing */)
            property Fact _motPwmMin: controller.getParameterFact(-1, _escPrefix + "PWM_MIN", false /* reportMissing */)
            property Fact _motPwmMax: controller.getParameterFact(-1, _escPrefix + "PWM_MAX", false /* reportMissing */)
            property Fact _motSpinArm: controller.getParameterFact(-1, _escPrefix + "SPIN_ARM", false /* reportMissing */)
            property Fact _motSpinMin: controller.getParameterFact(-1, _escPrefix + "SPIN_MIN", false /* reportMissing */)
            property Fact _motSpinMax: controller.getParameterFact(-1, _escPrefix + "SPIN_MAX", false /* reportMissing */)
            property Fact _servoDshotEsc: controller.getParameterFact(-1, "SERVO_DSHOT_ESC", false /* reportMissing */)
            property Fact _servoDshotRate: controller.getParameterFact(-1, "SERVO_DSHOT_RATE", false /* reportMissing */)

            property bool _isDshot: _motPwmTypeAvailable && _motPwmType && _motPwmType.rawValue >= 4

            property string _escCalParam: _isQuadPlane ? "Q_ESC_CAL" : "ESC_CALIBRATION"
            property bool _escCalibrationAvailable: controller.parameterExists(-1, _escCalParam)
            property Fact _escCalibration: controller.getParameterFact(-1, _escCalParam, false /* reportMissing */)

            property string _restartRequired: qsTr("Requires vehicle reboot")
            property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 15
            property real _comboWidth: ScreenTools.defaultFontPixelWidth * 30

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            QGCGroupBox {
                title: qsTr("Configuration")
                visible: _motPwmTypeAvailable

                ColumnLayout {
                    spacing: _margins

                    LabelledFactComboBox {
                        label: qsTr("Output type")
                        fact: _motPwmType
                        indexModel: false
                        comboBoxPreferredWidth: _comboWidth
                    }

                    QGCLabel {
                        text: _restartRequired
                        font.pointSize: ScreenTools.smallFontPointSize
                    }

                    LabelledFactTextField {
                        label: qsTr("Output PWM min")
                        fact: _motPwmMin
                        textFieldPreferredWidth: _fieldWidth
                        visible: _motPwmMinAvailable
                    }

                    LabelledFactTextField {
                        label: qsTr("Output PWM max")
                        fact: _motPwmMax
                        textFieldPreferredWidth: _fieldWidth
                        visible: _motPwmMaxAvailable
                    }

                    LabelledFactTextField {
                        label: qsTr("Spin when armed")
                        fact: _motSpinArm
                        textFieldPreferredWidth: _fieldWidth
                        visible: _motSpinArmAvailable
                    }

                    LabelledFactTextField {
                        label: qsTr("Spin minimum")
                        fact: _motSpinMin
                        textFieldPreferredWidth: _fieldWidth
                        visible: _motSpinMinAvailable
                    }

                    LabelledFactTextField {
                        label: qsTr("Spin maximum")
                        fact: _motSpinMax
                        textFieldPreferredWidth: _fieldWidth
                        visible: _motSpinMaxAvailable
                    }

                    // DShot settings - visible when a DShot protocol is selected
                    LabelledFactComboBox {
                        label: qsTr("DShot ESC type")
                        fact: _servoDshotEsc
                        indexModel: false
                        comboBoxPreferredWidth: _comboWidth
                        visible: _isDshot && _servoDshotEscAvailable
                    }

                    LabelledFactComboBox {
                        label: qsTr("DShot output rate")
                        fact: _servoDshotRate
                        indexModel: false
                        comboBoxPreferredWidth: _comboWidth
                        visible: _isDshot && _servoDshotRateAvailable
                    }
                }
            }

            QGCGroupBox {
                title: qsTr("Calibration")
                visible: _escCalibrationAvailable

                ColumnLayout {
                    spacing: _margins

                    QGCLabel {
                        text: qsTr("WARNING: Remove props prior to calibration!")
                        color: qgcPal.warningText
                    }

                    RowLayout {
                        spacing: _margins

                        QGCButton {
                            text: qsTr("Calibrate")
                            enabled: _escCalibration && _escCalibration.rawValue === 0
                            onClicked: if(_escCalibration) _escCalibration.rawValue = 3
                        }

                        ColumnLayout {
                            enabled: _escCalibration && _escCalibration.rawValue === 3
                            QGCLabel { text: _escCalibration ? (_escCalibration.rawValue === 3 ? qsTr("Now perform these steps:") : qsTr("Click Calibrate to start, then:")) : "" }
                            QGCLabel { text: qsTr("- Disconnect USB and battery so flight controller powers down") }
                            QGCLabel { text: qsTr("- Connect the battery") }
                            QGCLabel { text: qsTr("- The arming tone will be played (if the vehicle has a buzzer attached)") }
                            QGCLabel { text: qsTr("- If using a flight controller with a safety button press it until it displays solid red") }
                            QGCLabel { text: qsTr("- You will hear a musical tone then two beeps") }
                            QGCLabel { text: qsTr("- A few seconds later you should hear a number of beeps (one for each battery cell you're using)") }
                            QGCLabel { text: qsTr("- And finally a single long beep indicating the end points have been set and the ESC is calibrated") }
                            QGCLabel { text: qsTr("- Disconnect the battery and power up again normally") }
                        }
                    }
                }
            }
        } // ColumnLayout
    } // Component - escPageComponent

} // SetupPage
