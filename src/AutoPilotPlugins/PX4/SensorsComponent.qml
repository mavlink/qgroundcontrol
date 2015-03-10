/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    property QGCPalette qgcPal: QGCPalette { colorGroupEnabled: true }
    property ScreenTools screenTools: ScreenTools { }

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

    color: qgcPal.window

    // We use this bogus loader just so we can get an onLoaded signal to hook to in order to
    // finish controller initialization.
    Component {
        id: loadSignal;
        Item { }
    }
    Loader {
        sourceComponent: loadSignal
        onLoaded: {
            controller.statusLog = statusTextArea
            controller.progressBar = progressBar
        }
    }

    Column {
        anchors.fill: parent

        QGCLabel {
            text: "SENSORS CONFIG"
            font.pointSize: screenTools.dpiAdjustedPointSize(20);
        }

        Item { height: 20; width: 10 } // spacer

        Row {
            readonly property int buttonWidth: 120

            spacing: 20

            QGCLabel { text: "Calibrate:"; anchors.baseline: firstButton.baseline }

            IndicatorButton {
                property Fact fact: Fact { name: "CAL_MAG0_ID" }

                id:             firstButton
                width:          parent.buttonWidth
                text:           "Compass"
                indicatorGreen: fact.value != 0
                onClicked: controller.calibrateCompass()
            }

            IndicatorButton {
                property Fact fact: Fact { name: "CAL_GYRO0_ID" }

                width:          parent.buttonWidth
                text:           "Gyroscope"
                indicatorGreen: fact.value != 0
                onClicked: controller.calibrateGyro()
            }

            IndicatorButton {
                property Fact fact: Fact { name: "CAL_ACC0_ID" }

                width:          parent.buttonWidth
                text:           "Accelerometer"
                indicatorGreen: fact.value != 0
                onClicked: controller.calibrateAccel()
            }

            IndicatorButton {
                property Fact fact: Fact { name: "SENS_DPRES_OFF" }

                width:          parent.buttonWidth
                text:           "Airspeed"
                visible:        controller.fixedWing
                indicatorGreen: fact.value != 0
                onClicked:      controller.calibrateAirspeed()
            }
        }

        Item { height: 20; width: 10 } // spacer

        ProgressBar {
            id: progressBar
            width: parent.width - rotationColumnWidth
        }

        Item { height: 10; width: 10 } // spacer

        Item {
            readonly property int calibrationAreaHeight: 300
            property int calDisplayAreaWidth: parent.width - rotationColumnWidth

            width:  parent.width
            height: parent.height - y

            TextArea {
                id:             statusTextArea
                width:          parent.calDisplayAreaWidth
                height:         parent.height
                readOnly:       true
                frameVisible:   false
                text:           "Sensor config is a work in progress. Not all visuals for all calibration types fully implemented.\n\n" +
                                "For Compass calibration you will need to rotate your vehicle through a number of positions. For this calibration is is best " +
                                "to be connected to you vehicle via radio instead of USB since the USB cable will likely get in the way.\n\n" +
                                "For Gyroscope calibration you will need to place your vehicle right side up on solid surface and leave it still.\n\n" +
                                "For Accelerometer calibration you will need to place your vehicle on all six sides and hold it there for a few seconds.\n\n" +
                                "For Airspeed calibration you will need to keep your airspeed sensor out of any wind.\n\n"

                style: TextAreaStyle {
                    textColor: qgcPal.text
                    backgroundColor: qgcPal.windowShade
                }
            }

            Rectangle {
                id:         gyroCalArea
                width:      parent.calDisplayAreaWidth
                height:     parent.height
                visible:    controller.showGyroCalArea
                color:      qgcPal.windowShade

                Column {
                    width: parent.width

                    QGCLabel {
                        text: "Place your vehicle upright on a solid surface and hold it still."
                    }

                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       true
                        calInProgress:  controller.gyroCalInProgress
                        imageSource:    "qrc:///qml/VehicleDown.png"
                    }

                }
            }

            Rectangle {
                id:         accelCalArea
                width:      parent.calDisplayAreaWidth
                height:     parent.height
                visible:    controller.showAccelCalArea
                color:      qgcPal.windowShade

                QGCLabel {
                    id:         calAreaLabel
                    width:      parent.width
                    wrapMode:   Text.WordWrap

                    text: "Place your vehicle into each of the positions below and hold still. Once that position is completed you can move to another."
                }

                Flow {
                    y:          calAreaLabel.height
                    width:      parent.width
                    height:     parent.height - calAreaLabel.implicitHeight
                    spacing:    5

                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalDownSideDone
                        calInProgress:  controller.accelCalDownSideInProgress
                        imageSource:    "qrc:///qml/VehicleDown.png"
                    }
                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalUpsideDownSideDone
                        calInProgress:  controller.accelCalUpsideDownSideInProgress
                        imageSource:    "qrc:///qml/VehicleUpsideDown.png"
                    }
                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalNoseDownSideDone
                        calInProgress:  controller.accelCalNoseDownSideInProgress
                        imageSource:    "qrc:///qml/VehicleNoseDown.png"
                    }
                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalTailDownSideDone
                        calInProgress:  controller.accelCalTailDownSideInProgress
                        imageSource:    "qrc:///qml/VehicleTailDown.png"
                    }
                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalLeftSideDone
                        calInProgress:  controller.accelCalLeftSideInProgress
                        imageSource:    "qrc:///qml/VehicleLeft.png"
                    }
                    VehicleRotationCal {
                        width:          200
                        height:         200
                        calValid:       controller.accelCalRightSideDone
                        calInProgress:  controller.accelCalRightSideInProgress
                        imageSource:    "qrc:///qml/VehicleRight.png"
                    }
                }
            }

            Column {
                property Fact cal_mag0_rot: Fact { name: "CAL_MAG0_ROT" }
                property Fact cal_mag1_rot: Fact { name: "CAL_MAG1_ROT" }
                property Fact cal_mag2_rot: Fact { name: "CAL_MAG2_ROT" }

                // Compass rotation parameter < 0 indicates either internal compass, or no compass. So in
                // both those cases we do not show a rotation combo.
                property bool showCompass0: cal_mag0_rot.value >= 0
                property bool showCompass1: cal_mag1_rot.value >= 0
                property bool showCompass2: cal_mag2_rot.value >= 0

                x: parent.width - rotationColumnWidth

                QGCLabel { text: "Autpilot Orientation" }

                FactComboBox {
                    width:  rotationColumnWidth;
                    model:  rotations
                    fact:   Fact { name: "SENS_BOARD_ROT" }
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
                        fact:   Fact { name: "CAL_MAG0_ROT" }
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
                        fact:   Fact { name: "CAL_MAG1_ROT" }
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
                        fact:   Fact { name: "CAL_MAG2_ROT" }
                    }
                }
                Loader { sourceComponent: parent.showCompass2 ? compass2ComponentLabel : null }
                Loader { sourceComponent: parent.showCompass2 ? compass2ComponentCombo : null }
            }
        }
    }
}

