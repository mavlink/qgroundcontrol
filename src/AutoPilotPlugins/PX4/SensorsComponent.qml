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
import QtQuick.Dialogs 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0

QGCView {
    id:             rootQGCView
    viewComponent:  view

    // Help text which is shown both in the status text area prior to pressing a cal button and in the
    // pre-calibration dialog.

    readonly property string boardRotationText: "If the autopilot is mounted in flight direction, leave the default value (ROTATION_NONE)"
    readonly property string compassRotationText: "If the compass or GPS module is mounted in flight direction, leave the default value (ROTATION_NONE)"

    readonly property string compassHelp:   "For Compass calibration you will need to rotate your vehicle through a number of positions. Most users prefer to do this wirelessly with the telemetry link."
    readonly property string gyroHelp:      "For Gyroscope calibration you will need to place your vehicle on a surface and leave it still."
    readonly property string accelHelp:     "For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds."
    readonly property string levelHelp:     "To level the horizon you need to place the vehicle in its level flight position and press OK."
    readonly property string airspeedHelp:  "For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor."

    readonly property string statusTextAreaDefaultText: "Start the individual calibration steps by clicking one of the buttons above."

    // Used to pass what type of calibration is being performed to the preCalibrationDialog
    property string preCalibrationDialogType

    // Used to pass help text to the preCalibrationDialog dialog
    property string preCalibrationDialogHelp

    readonly property int sideBarH1PointSize: (ScreenTools.defaultFontPointSize * 1.3 + 0.5)
    readonly property int mainTextH1PointSize: (ScreenTools.defaultFontPointSize * 1.5 + 0.5)

    readonly property int rotationColumnWidth: 300
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

    Fact { id: cal_mag0_id; name: "CAL_MAG0_ID"; onFactMissing: showMissingFactOverlay(name) }
    Fact { id: cal_mag1_id; name: "CAL_MAG1_ID"; onFactMissing: showMissingFactOverlay(name) }
    Fact { id: cal_mag2_id; name: "CAL_MAG2_ID"; onFactMissing: showMissingFactOverlay(name) }
    Fact { id: cal_mag0_rot; name: "CAL_MAG0_ROT"; onFactMissing: showMissingFactOverlay(name) }
    Fact { id: cal_mag1_rot; name: "CAL_MAG1_ROT"; onFactMissing: showMissingFactOverlay(name) }
    Fact { id: cal_mag2_rot; name: "CAL_MAG2_ROT"; onFactMissing: showMissingFactOverlay(name) }

    // Id > = signals compass available, rot < 0 signals internal compass
    property bool showCompass0Rot: cal_mag0_id.value > 0 && cal_mag0_rot.value >= 0
    property bool showCompass1Rot: cal_mag1_id.value > 0 && cal_mag1_rot.value >= 0
    property bool showCompass2Rot: cal_mag2_id.value > 0 && cal_mag2_rot.value >= 0

    SensorsComponentController {
        id: controller

        onResetStatusTextArea: statusLog.text = statusTextAreaDefaultText

        onSetCompassRotations: {
            if (showCompass0Rot || showCompass1Rot || showCompass2Rot) {
            showDialog(compassRotationDialogComponent, "Set Compass Rotation(s)", 50, StandardButton.Ok)
            }
        }

        onWaitingForCancelChanged: {
            if (controller.waitingForCancel) {
                showMessage("Calibration Cancel", "Waiting for Vehicle to response to Cancel. This may take a few seconds.", 0)
            } else {
                hideDialog()
            }
        }
    }

    Component {
        id: preCalibrationDialogComponent

        QGCViewDialog {
            id: preCalibrationDialog

            function accept() {
                if (preCalibrationDialogType == "gyro") {
                    controller.calibrateGyro()
                } else if (preCalibrationDialogType == "accel") {
                    controller.calibrateAccel()
                } else if (preCalibrationDialogType == "level") {
                    controller.calibrateLevel()
                } else if (preCalibrationDialogType == "compass") {
                    controller.calibrateCompass()
                } else if (preCalibrationDialogType == "airspeed") {
                    controller.calibrateAirspeed()
                }
                preCalibrationDialog.hideDialog()
            }

            Column {
                anchors.fill: parent
                spacing:                5

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       preCalibrationDialogHelp
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    visible:    (preCalibrationDialogType != "airspeed") && (preCalibrationDialogType != "gyro")
                    text:       boardRotationText
                }

                FactComboBox {
                    width:      rotationColumnWidth
                    model:      rotations
                    visible:    preCalibrationDialogType != "airspeed" && (preCalibrationDialogType != "gyro")
                    fact:       Fact { name: "SENS_BOARD_ROT"; onFactMissing: showMissingFactOverlay(name) }
                }
            }
        }
    }

    Component {
        id: compassRotationDialogComponent

        QGCViewDialog {
            id: compassRotationDialog

            Column {
                anchors.fill: parent
                spacing:      10

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       compassRotationText
                }

                // Compass 0 rotation
                Component {
                    id: compass0ComponentLabel

                    QGCLabel {
                        font.pointSize: sideBarH1PointSize
                        text: "External Compass Orientation"
                    }

                }
                Component {
                    id: compass0ComponentCombo

                    FactComboBox {
                        id:     compass0RotationCombo
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   Fact { name: "CAL_MAG0_ROT"; onFactMissing: showMissingFactOverlay(name) }
                    }
                }
                Loader { sourceComponent: showCompass0Rot ? compass0ComponentLabel : null }
                Loader { sourceComponent: showCompass0Rot ? compass0ComponentCombo : null }
                // Compass 1 rotation
                Component {
                    id: compass1ComponentLabel

                    QGCLabel { text: "Compass 1 Orientation" }
                }
                Component {
                    id: compass1ComponentCombo

                    FactComboBox {
                        id:     compass1RotationCombo
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   Fact { name: "CAL_MAG1_ROT"; onFactMissing: showMissingFactOverlay(name) }
                    }
                }
                Loader { sourceComponent: showCompass1Rot ? compass1ComponentLabel : null }
                Loader { sourceComponent: showCompass1Rot ? compass1ComponentCombo : null }

                // Compass 2 rotation
                Component {
                    id: compass2ComponentLabel

                    QGCLabel { text: "Compass 2 Orientation" }
                }
                Component {
                    id: compass2ComponentCombo

                    FactComboBox {
                        id:     compass1RotationCombo
                        width:  rotationColumnWidth
                        model:  rotations
                        fact:   Fact { name: "CAL_MAG2_ROT"; onFactMissing: showMissingFactOverlay(name) }
                    }
                }
                Loader { sourceComponent: showCompass2Rot ? compass2ComponentLabel : null }
                Loader { sourceComponent: showCompass2Rot ? compass2ComponentCombo : null }
            } // Column
        } // QGCViewDialog
    } // Component - compassRotationDialogComponent

    Component {
        id: view

        QGCViewPanel {
            id:             viewPanel

            Connections {
                target: rootQGCView

                onCompleted: {
                    controller.factPanel = viewPanel
                    controller.statusLog = statusTextArea
                    controller.progressBar = progressBar
                    controller.compassButton = compassButton
                    controller.gyroButton = gyroButton
                    controller.accelButton = accelButton
                    controller.airspeedButton = airspeedButton
                    controller.levelButton = levelButton
                    controller.cancelButton = cancelButton
                    controller.orientationCalAreaHelpText = orientationCalAreaHelpText
                }
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: enabled }



            color: qgcPal.window


            Column {
                anchors.fill: parent

                QGCLabel {
                    text: "SENSORS CONFIG"
                    font.pointSize: ScreenTools.fontPointFactor * (20);
                }

                Item { height: 20; width: 10 } // spacer

                Row {
                    readonly property int buttonWidth: 120

                    spacing: 20

                    QGCLabel { text: "Calibrate:"; anchors.baseline: compassButton.baseline }

                    IndicatorButton {
                        property Fact fact: Fact { name: "CAL_MAG0_ID" }

                        id:             compassButton
                        width:          parent.buttonWidth
                        text:           "Compass"
                        indicatorGreen: fact.value != 0

                        onClicked: {
                            preCalibrationDialogType = "compass"
                            preCalibrationDialogHelp = compassHelp
                            showDialog(preCalibrationDialogComponent, "Calibrate Compass", 50, StandardButton.Cancel | StandardButton.Ok)
                        }
                    }

                    IndicatorButton {
                        property Fact fact: Fact { name: "CAL_GYRO0_ID" }

                        id:             gyroButton
                        width:          parent.buttonWidth
                        text:           "Gyroscope"
                        indicatorGreen: fact.value != 0

                        onClicked: {
                            preCalibrationDialogType = "gyro"
                            preCalibrationDialogHelp = gyroHelp
                            showDialog(preCalibrationDialogComponent, "Calibrate Gyro", 50, StandardButton.Cancel | StandardButton.Ok)
                        }
                    }

                    IndicatorButton {
                        property Fact fact: Fact { name: "CAL_ACC0_ID" }

                        id:             accelButton
                        width:          parent.buttonWidth
                        text:           "Accelerometer"
                        indicatorGreen: fact.value != 0

                        onClicked: {
                            preCalibrationDialogType = "accel"
                            preCalibrationDialogHelp = accelHelp
                            showDialog(preCalibrationDialogComponent, "Calibrate Accelerometer", 50, StandardButton.Cancel | StandardButton.Ok)
                        }
                    }

                    IndicatorButton {
                        property Fact fact: Fact { name: "SENS_BOARD_X_OFF" }
                        property Fact checkAcc: Fact { name: "CAL_ACC0_ID" }
                        property Fact checkGyro: Fact { name: "CAL_GYRO0_ID" }

                        id:             levelButton
                        width:          parent.buttonWidth
                        text:           "Level Horizon"
                        indicatorGreen: fact.value != 0
                        enabled: checkAcc.value != 0 && checkGyro.value != 0

                        onClicked: {
                            preCalibrationDialogType = "level"
                            preCalibrationDialogHelp = levelHelp
                            showDialog(preCalibrationDialogComponent, "Level Horizon", 50, StandardButton.Cancel | StandardButton.Ok)
                        }
                    }

                    IndicatorButton {
                        property Fact fact: Fact { name: "SENS_DPRES_OFF" }

                        id:             airspeedButton
                        width:          parent.buttonWidth
                        text:           "Airspeed"
                        visible:        controller.fixedWing
                        indicatorGreen: fact.value != 0

                        onClicked: {
                            preCalibrationDialogType = "airspeed"
                            preCalibrationDialogHelp = airspeedHelp
                            showDialog(preCalibrationDialogComponent, "Calibrate Airspeed", 50, StandardButton.Cancel | StandardButton.Ok)
                        }
                    }

                    QGCButton {
                        id:         cancelButton
                        text:       "Cancel"
                        enabled:    false
                        onClicked:  controller.cancelCalibration()
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
                        text:           statusTextAreaDefaultText

                        style: TextAreaStyle {
                            textColor: qgcPal.text
                            backgroundColor: qgcPal.windowShade
                        }
                    }

                    Rectangle {
                        id:         orientationCalArea
                        width:      parent.calDisplayAreaWidth
                        height:     parent.height
                        visible:    controller.showOrientationCalArea
                        color:      qgcPal.windowShade

                        QGCLabel {
                            id:             orientationCalAreaHelpText
                            width:          parent.width
                            wrapMode:       Text.WordWrap
                            font.pointSize: ScreenTools.fontPointFactor * (17);
                        }

                        Flow {
                            y:          orientationCalAreaHelpText.height
                            width:      parent.width
                            height:     parent.height - orientationCalAreaHelpText.implicitHeight
                            spacing:    5

                            VehicleRotationCal {
                                visible:            controller.orientationCalDownSideVisible
                                calValid:           controller.orientationCalDownSideDone
                                calInProgress:      controller.orientationCalDownSideInProgress
                                calInProgressText:  controller.orientationCalDownSideRotate ? "Rotate" : "Hold Still"
                                imageSource:        controller.orientationCalDownSideRotate ? "qrc:///qml/VehicleDownRotate.png" : "qrc:///qml/VehicleDown.png"
                            }
                            VehicleRotationCal {
                                visible:            controller.orientationCalUpsideDownSideVisible
                                calValid:           controller.orientationCalUpsideDownSideDone
                                calInProgress:      controller.orientationCalUpsideDownSideInProgress
                                calInProgressText:  "Hold Still"
                                imageSource:        "qrc:///qml/VehicleUpsideDown.png"
                            }
                            VehicleRotationCal {
                                visible:            controller.orientationCalNoseDownSideVisible
                                calValid:           controller.orientationCalNoseDownSideDone
                                calInProgress:      controller.orientationCalNoseDownSideInProgress
                                calInProgressText:  controller.orientationCalNoseDownSideRotate ? "Rotate" : "Hold Still"
                                imageSource:        controller.orientationCalNoseDownSideRotate ? "qrc:///qml/VehicleNoseDownRotate.png" : "qrc:///qml/VehicleNoseDown.png"
                            }
                            VehicleRotationCal {
                                visible:            controller.orientationCalTailDownSideVisible
                                calValid:           controller.orientationCalTailDownSideDone
                                calInProgress:      controller.orientationCalTailDownSideInProgress
                                calInProgressText:  "Hold Still"
                                imageSource:        "qrc:///qml/VehicleTailDown.png"
                            }
                            VehicleRotationCal {
                                visible:            controller.orientationCalLeftSideVisible
                                calValid:           controller.orientationCalLeftSideDone
                                calInProgress:      controller.orientationCalLeftSideInProgress
                                calInProgressText:  controller.orientationCalLeftSideRotate ? "Rotate" : "Hold Still"
                                imageSource:        controller.orientationCalLeftSideRotate ? "qrc:///qml/VehicleLeftRotate.png" : "qrc:///qml/VehicleLeft.png"
                            }
                            VehicleRotationCal {
                                visible:            controller.orientationCalRightSideVisible
                                calValid:           controller.orientationCalRightSideDone
                                calInProgress:      controller.orientationCalRightSideInProgress
                                calInProgressText:  "Hold Still"
                                imageSource:        "qrc:///qml/VehicleRight.png"
                            }
                        }
                    }

                    Column {
                        anchors.left: orientationCalArea.right
                        anchors.leftMargin: 5
                        spacing: 20
                        x: parent.width - rotationColumnWidth

                        Column {
                            spacing:            5

                            QGCLabel {
                                font.pointSize: sideBarH1PointSize
                                text: "Autopilot Orientation"
                            }

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text: boardRotationText
                            }

                            FactComboBox {
                                id:     boardRotationCombo
                                width:  rotationColumnWidth;
                                model:  rotations
                                fact:   Fact { name: "SENS_BOARD_ROT" }
                            }
                        }

                        Column {
                            spacing:            5

                            // Compass 0 rotation
                            Component {
                                id: compass0ComponentLabel2

                                QGCLabel {
                                    font.pointSize: sideBarH1PointSize
                                    text: "External Compass Orientation"
                                }
                            }
                            Component {
                                id: compass0ComponentCombo2

                                FactComboBox {
                                    id:     compass0RotationCombo
                                    width:  rotationColumnWidth
                                    model:  rotations
                                    fact:   Fact { name: "CAL_MAG0_ROT" }
                                }
                            }
                            Loader { sourceComponent: showCompass0Rot ? compass0ComponentLabel2 : null }
                            Loader { sourceComponent: showCompass0Rot ? compass0ComponentCombo2 : null }
                        }

                        Column {
                            spacing:            5
                            // Compass 1 rotation
                            Component {
                                id: compass1ComponentLabel2

                                QGCLabel {
                                    font.pointSize: sideBarH1PointSize
                                    text: "External Compass 1 Orientation"
                                }
                            }
                            Component {
                                id: compass1ComponentCombo2

                                FactComboBox {
                                    id:     compass1RotationCombo
                                    width:  rotationColumnWidth
                                    model:  rotations
                                    fact:   Fact { name: "CAL_MAG1_ROT" }
                                }
                            }
                            Loader { sourceComponent: showCompass1Rot ? compass1ComponentLabel2 : null }
                            Loader { sourceComponent: showCompass1Rot ? compass1ComponentCombo2 : null }
                        }

                        Column {
                            spacing:            5
                            // Compass 2 rotation
                            Component {
                                id: compass2ComponentLabel2

                                QGCLabel {
                                    font.pointSize: sidebarH1PointSize
                                    text: "Compass 2 Orientation"
                                }
                            }
                            Component {
                                id: compass2ComponentCombo2

                                FactComboBox {
                                    id:     compass1RotationCombo
                                    width:  rotationColumnWidth
                                    model:  rotations
                                    fact:   Fact { name: "CAL_MAG2_ROT" }
                                }
                            }
                            Loader { sourceComponent: showCompass2Rot ? compass2ComponentLabel2 : null }
                            Loader { sourceComponent: showCompass2Rot ? compass2ComponentCombo2 : null }
                        }
                    }
                }
            }
        } // Rectangle
    } // Component - view
}
