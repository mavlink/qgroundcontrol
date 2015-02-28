import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

Rectangle {
    property QGCPalette qgcPal: QGCPalette { colorGroupEnabled: true }

    readonly property int rotationColumnWidth: 200
    readonly property var rotations: [
        "ROTATION_NONE",
        "ROTATION_YAW_45",
        "ROTATION_YAW_90",
        "ROTATION_YAW_135",
        "ROTATION_YAW_180",
        "ROTATION_YAW_225",
        "ROTATION_YAW_270",
        "ROTATION_YAW_315",
        "ROTATION_ROLL_180",
        "ROTATION_ROLL_180_YAW_45",
        "ROTATION_ROLL_180_YAW_90",
        "ROTATION_ROLL_180_YAW_135",
        "ROTATION_PITCH_180",
        "ROTATION_ROLL_180_YAW_225",
        "ROTATION_ROLL_180_YAW_270",
        "ROTATION_ROLL_180_YAW_315",
        "ROTATION_ROLL_90",
        "ROTATION_ROLL_90_YAW_45",
        "ROTATION_ROLL_90_YAW_90",
        "ROTATION_ROLL_90_YAW_135",
        "ROTATION_ROLL_270",
        "ROTATION_ROLL_270_YAW_45",
        "ROTATION_ROLL_270_YAW_90",
        "ROTATION_ROLL_270_YAW_135",
        "ROTATION_PITCH_90",
        "ROTATION_PITCH_270",
        "ROTATION_ROLL_270_YAW_270"
    ]

    width: 600
    height: 600
    color: qgcPal.window

    // We use this bogus loader just so we can get an onLoaded signal to hook to in order to
    // finish controller initialization.
    Component {
        id: loadSignal;
        Item { }
    }
    Loader {
        sourceComponent: loadSignal
        onLoaded: controller.statusLog = statusTextArea
    }

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "SENSORS CONFIG"
            font.pointSize: 20
        }

        Item { height: 20; width: 10 } // spacer

        Item {
            readonly property int calibrationAreaHeight: 300

            width:  parent.width
            height: calibrationAreaHeight

            TextArea {
                id:             statusTextArea
                width:          parent.width - rotationColumnWidth
                height:         parent.height
                readOnly:       true
                frameVisible:   false
                text:           qsTr("Sensor config is a work in progress which currently supports textual instructions only. Updated visuals coming soon.")

                style: TextAreaStyle {
                    textColor: qgcPal.text
                    backgroundColor: qgcPal.windowShade
                }
            }

            Column {
                // Compass rotation parameter < 0 indicates either internal compass, or no compass. So in
                // both those cases we do not show a rotation combo.
                property bool showCompass0: autopilot.parameters["CAL_MAG0_ROT"].value >= 0
                property bool showCompass1: autopilot.parameters["CAL_MAG1_ROT"].value >= 0
                property bool showCompass2: autopilot.parameters["CAL_MAG2_ROT"].value >= 0

                x: parent.width - rotationColumnWidth

                QGCLabel { text: "Autpilot Orientation" }

                FactComboBox {
                    width:  rotationColumnWidth;
                    model:  rotations
                    fact:   autopilot.parameters["SENS_BOARD_ROT"]
                }

                // Compass 0 rotation
                Component {
                    id: compass0ComponentLabel

                    QGCLabel { text: "Compass Orientation" }
                }
                Component {
                    id: compass0ComponentCombo

                    FactComboBox {
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   autopilot.parameters["CAL_MAG0_ROT"]
                    }
                }
                Loader { sourceComponent: parent.showCompass0 ? compass0ComponentLabel : null }
                Loader { sourceComponent: parent.showCompass0 ? compass0ComponentCombo : null }

                // Compass 1 rotation
                Component {
                    id: compass1ComponentLabel

                    QGCLabel { text: "Compass 1 Orientation" }
                }
                Component {
                    id: compass1ComponentCombo

                    FactComboBox {
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   autopilot.parameters["CAL_MAG1_ROT"]
                    }
                }
                Loader { sourceComponent: parent.showCompass1 ? compass1ComponentLabel : null }
                Loader { sourceComponent: parent.showCompass1 ? compass1ComponentCombo : null }

                // Compass 2 rotation
                Component {
                    id: compass2ComponentLabel

                    QGCLabel { text: "Compass 2 Orientation" }
                }
                Component {
                    id: compass2ComponentCombo

                    FactComboBox {
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   autopilot.parameters["CAL_MAG2_ROT"]
                    }
                }
                Loader { sourceComponent: parent.showCompass2 ? compass2ComponentLabel : null }
                Loader { sourceComponent: parent.showCompass2 ? compass2ComponentCombo : null }
            }
        }

        Item { height: 20; width: 10 } // spacer

        Row {
            readonly property int buttonWidth: 120

            spacing: 20

            QGCLabel { text: "Calibrate:"; anchors.baseline: firstButton.baseline }

            IndicatorButton {
                id:             firstButton
                width:          parent.buttonWidth
                text:           "Compass"
                indicatorGreen: autopilot.parameters["CAL_MAG0_ID"].value != 0
                onClicked: controller.calibrateCompass()
            }

            IndicatorButton {
                width:          parent.buttonWidth
                text:           "Gyroscope"
                indicatorGreen: autopilot.parameters["CAL_GYRO0_ID"].value != 0
                onClicked: controller.calibrateGyro()
            }

            IndicatorButton {
                width:          parent.buttonWidth
                text:           "Acceleromter"
                indicatorGreen: autopilot.parameters["CAL_ACC0_ID"].value != 0
                onClicked: controller.calibrateAccel()
            }

            IndicatorButton {
                width:          parent.buttonWidth
                text:           "Airspeed"
                visible:        controller.fixedWing
                indicatorGreen: autopilot.parameters["SENS_DPRES_OFF"].value != 0
                onClicked: controller.calibrateAirspeed()
            }
        }
    }
}

