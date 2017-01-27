/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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

SetupPage {
    id:             sensorsPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Item {
            width:  availableWidth
            height: availableHeight

            // Help text which is shown both in the status text area prior to pressing a cal button and in the
            // pre-calibration dialog.

            readonly property string boardRotationText: qsTr("If the orientation is in the direction of flight, select ROTATION_NONE.")
            readonly property string compassRotationText: qsTr("If the orientation is in the direction of flight, select ROTATION_NONE.")

            readonly property string compassHelp:   qsTr("For Compass calibration you will need to rotate your vehicle through a number of positions.")
            readonly property string gyroHelp:      qsTr("For Gyroscope calibration you will need to place your vehicle on a surface and leave it still.")
            readonly property string accelHelp:     qsTr("For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds.")
            readonly property string levelHelp:     qsTr("To level the horizon you need to place the vehicle in its level flight position and press OK.")
            readonly property string airspeedHelp:  qsTr("For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor.")

            readonly property string statusTextAreaDefaultText: qsTr("Start the individual calibration steps by clicking one of the buttons to the left.")

            // Used to pass what type of calibration is being performed to the preCalibrationDialog
            property string preCalibrationDialogType

            // Used to pass help text to the preCalibrationDialog dialog
            property string preCalibrationDialogHelp

            readonly property int rotationColumnWidth: ScreenTools.defaultFontPixelWidth * 30
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
                "ROTATION_ROLL_270_YAW_270",
                "ROTATION_ROLL_180_PITCH_270",
                "ROTATION_PITCH_90_YAW_180",
                "ROTATION_ROLL_90_PITCH_90"
            ]

            property Fact cal_mag0_id:      controller.getParameterFact(-1, "CAL_MAG0_ID")
            property Fact cal_mag1_id:      controller.getParameterFact(-1, "CAL_MAG1_ID")
            property Fact cal_mag2_id:      controller.getParameterFact(-1, "CAL_MAG2_ID")
            property Fact cal_mag0_rot:     controller.getParameterFact(-1, "CAL_MAG0_ROT")
            property Fact cal_mag1_rot:     controller.getParameterFact(-1, "CAL_MAG1_ROT")
            property Fact cal_mag2_rot:     controller.getParameterFact(-1, "CAL_MAG2_ROT")

            property Fact cal_gyro0_id:     controller.getParameterFact(-1, "CAL_GYRO0_ID")
            property Fact cal_acc0_id:      controller.getParameterFact(-1, "CAL_ACC0_ID")

            property Fact sens_board_rot:   controller.getParameterFact(-1, "SENS_BOARD_ROT")
            property Fact sens_board_x_off: controller.getParameterFact(-1, "SENS_BOARD_X_OFF")
            property Fact sens_board_y_off: controller.getParameterFact(-1, "SENS_BOARD_Y_OFF")
            property Fact sens_board_z_off: controller.getParameterFact(-1, "SENS_BOARD_Z_OFF")
            property Fact sens_dpres_off:   controller.getParameterFact(-1, "SENS_DPRES_OFF")

            // Id > = signals compass available, rot < 0 signals internal compass
            property bool showCompass0Rot: cal_mag0_id.value > 0 && cal_mag0_rot.value >= 0
            property bool showCompass1Rot: cal_mag1_id.value > 0 && cal_mag1_rot.value >= 0
            property bool showCompass2Rot: cal_mag2_id.value > 0 && cal_mag2_rot.value >= 0

            SensorsComponentController {
                id:                         controller
                factPanel:                  sensorsPage.viewPanel
                statusLog:                  statusTextArea
                progressBar:                progressBar
                compassButton:              compassButton
                gyroButton:                 gyroButton
                accelButton:                accelButton
                airspeedButton:             airspeedButton
                levelButton:                levelButton
                cancelButton:               cancelButton
                setOrientationsButton:      setOrientationsButton
                orientationCalAreaHelpText: orientationCalAreaHelpText

                onResetStatusTextArea: statusLog.text = statusTextAreaDefaultText

                onSetCompassRotations: {
                    if (showCompass0Rot || showCompass1Rot || showCompass2Rot) {
                        setOrientationsDialogShowBoardOrientation = false
                        showDialog(setOrientationsDialogComponent, qsTr("Set Compass Rotation(s)"), sensorsPage.showDialogDefaultWidth, StandardButton.Ok)
                    }
                }

                onWaitingForCancelChanged: {
                    if (controller.waitingForCancel) {
                        showMessage(qsTr("Calibration Cancel"), qsTr("Waiting for Vehicle to response to Cancel. This may take a few seconds."), 0)
                    } else {
                        hideDialog()
                    }
                }

            }

            Component.onCompleted: {
                var usingUDP = controller.usingUDPLink()
                if (usingUDP) {
                    console.log("onUsingUDPLink")
                    showMessage("Sensor Calibration", "Performing sensor calibration over a WiFi connection is known to be unreliable. You should disconnect and perform calibration using a direct USB connection instead.", StandardButton.Ok)
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
                        anchors.fill:   parent
                        spacing:        5

                        QGCLabel {
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            text:       preCalibrationDialogHelp
                        }

                        QGCLabel {
                            id:         boardRotationHelp
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            visible:    (preCalibrationDialogType != "airspeed") && (preCalibrationDialogType != "gyro")
                            text:       boardRotationText
                        }

                        Column {
                            visible:    boardRotationHelp.visible
                            QGCLabel {
                                text: qsTr("Autopilot Orientation:")
                            }

                            FactComboBox {
                                id:     boardRotationCombo
                                width:  rotationColumnWidth;
                                model:  rotations
                                fact:   sens_board_rot
                            }
                        }
                    }
                }
            }

            property bool setOrientationsDialogShowBoardOrientation: true

            Component {
                id: setOrientationsDialogComponent

                QGCViewDialog {
                    id: setOrientationsDialog

                    QGCFlickable {
                        anchors.fill:   parent
                        contentHeight:  columnLayout.height
                        clip:           true

                        Column {
                            id:                 columnLayout
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        parent.top
                            spacing:            ScreenTools.defaultFontPixelHeight

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text:       boardRotationText
                            }

                            Column {
                                visible: setOrientationsDialogShowBoardOrientation

                                QGCLabel {
                                    text: qsTr("Autopilot Orientation:")
                                }

                                FactComboBox {
                                    id:     boardRotationCombo
                                    width:  rotationColumnWidth;
                                    model:  rotations
                                    fact:   sens_board_rot
                                }
                            }

                            Column {
                                // Compass 0 rotation
                                Component {
                                    id: compass0ComponentLabel2

                                    QGCLabel {
                                        text: qsTr("External Compass Orientation:")
                                    }
                                }

                                Component {
                                    id: compass0ComponentCombo2

                                    FactComboBox {
                                        id:     compass0RotationCombo
                                        width:  rotationColumnWidth
                                        model:  rotations
                                        fact:   cal_mag0_rot
                                    }
                                }

                                Loader { sourceComponent: showCompass0Rot ? compass0ComponentLabel2 : null }
                                Loader { sourceComponent: showCompass0Rot ? compass0ComponentCombo2 : null }
                            }

                            Column {
                                // Compass 1 rotation
                                Component {
                                    id: compass1ComponentLabel2

                                    QGCLabel {
                                        text: qsTr("External Compass 1 Orientation:")
                                    }
                                }

                                Component {
                                    id: compass1ComponentCombo2

                                    FactComboBox {
                                        id:     compass1RotationCombo
                                        width:  rotationColumnWidth
                                        model:  rotations
                                        fact:   cal_mag1_rot
                                    }
                                }

                                Loader { sourceComponent: showCompass1Rot ? compass1ComponentLabel2 : null }
                                Loader { sourceComponent: showCompass1Rot ? compass1ComponentCombo2 : null }
                            }

                            Column {
                                spacing: ScreenTools.defaultFontPixelWidth

                                // Compass 2 rotation
                                Component {
                                    id: compass2ComponentLabel2

                                    QGCLabel {
                                        text: qsTr("Compass 2 Orientation")
                                    }
                                }

                                Component {
                                    id: compass2ComponentCombo2

                                    FactComboBox {
                                        id:     compass1RotationCombo
                                        width:  rotationColumnWidth
                                        model:  rotations
                                        fact:   cal_mag2_rot
                                    }
                                }
                                Loader { sourceComponent: showCompass2Rot ? compass2ComponentLabel2 : null }
                                Loader { sourceComponent: showCompass2Rot ? compass2ComponentCombo2 : null }
                            }
                        } // Column
                    } // QGCFlickable
                } // QGCViewDialog
            } // Component - setOrientationsDialogComponent

            Column {
                id:         buttonColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2

                readonly property int buttonWidth: ScreenTools.defaultFontPixelWidth * 15

                IndicatorButton {
                    id:             compassButton
                    width:          parent.buttonWidth
                    text:           qsTr("Compass")
                    indicatorGreen: cal_mag0_id.value != 0

                    onClicked: {
                        preCalibrationDialogType = "compass"
                        preCalibrationDialogHelp = compassHelp
                        showDialog(preCalibrationDialogComponent, qsTr("Calibrate Compass"), sensorsPage.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }

                IndicatorButton {
                    id:             gyroButton
                    width:          parent.buttonWidth
                    text:           qsTr("Gyroscope")
                    indicatorGreen: cal_gyro0_id.value != 0

                    onClicked: {
                        preCalibrationDialogType = "gyro"
                        preCalibrationDialogHelp = gyroHelp
                        showDialog(preCalibrationDialogComponent, qsTr("Calibrate Gyro"), sensorsPage.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }

                IndicatorButton {
                    id:             accelButton
                    width:          parent.buttonWidth
                    text:           qsTr("Accelerometer")
                    indicatorGreen: cal_acc0_id.value != 0

                    onClicked: {
                        preCalibrationDialogType = "accel"
                        preCalibrationDialogHelp = accelHelp
                        showDialog(preCalibrationDialogComponent, qsTr("Calibrate Accelerometer"), sensorsPage.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }

                IndicatorButton {
                    id:             levelButton
                    width:          parent.buttonWidth
                    text:           qsTr("Level Horizon")
                    indicatorGreen: sens_board_x_off.value != 0 || sens_board_y_off != 0 | sens_board_z_off != 0
                    enabled:        cal_acc0_id.value != 0 && cal_gyro0_id.value != 0

                    onClicked: {
                        preCalibrationDialogType = "level"
                        preCalibrationDialogHelp = levelHelp
                        showDialog(preCalibrationDialogComponent, qsTr("Level Horizon"), sensorsPage.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }

                IndicatorButton {
                    id:             airspeedButton
                    width:          parent.buttonWidth
                    text:           qsTr("Airspeed")
                    visible:        (controller.vehicle.fixedWing || controller.vehicle.vtol) && controller.getParameterFact(-1, "CBRK_AIRSPD_CHK").value != 162128
                    indicatorGreen: sens_dpres_off.value != 0

                    onClicked: {
                        preCalibrationDialogType = "airspeed"
                        preCalibrationDialogHelp = airspeedHelp
                        showDialog(preCalibrationDialogComponent, qsTr("Calibrate Airspeed"), sensorsPage.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }
                }

                QGCButton {
                    id:         cancelButton
                    width:      parent.buttonWidth
                    text:       qsTr("Cancel")
                    enabled:    false
                    onClicked:  controller.cancelCalibration()
                }

                QGCButton {
                    id:         setOrientationsButton
                    width:      parent.buttonWidth
                    text:       qsTr("Set Orientations")
                    onClicked:  {
                        setOrientationsDialogShowBoardOrientation = true
                        showDialog(setOrientationsDialogComponent, qsTr("Set Orientations"), sensorsPage.showDialogDefaultWidth, StandardButton.Ok)
                    }
                }
            } // Column - Buttons

            Column {
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                anchors.left:       buttonColumn.right
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom

                ProgressBar {
                    id:             progressBar
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 10 } // spacer

                Item {
                    property int calDisplayAreaWidth: parent.width

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
                            id:                 orientationCalAreaHelpText
                            anchors.margins:    ScreenTools.defaultFontPixelWidth
                            anchors.top:        orientationCalArea.top
                            anchors.left:       orientationCalArea.left
                            width:              parent.width
                            wrapMode:           Text.WordWrap
                            font.pointSize:     ScreenTools.mediumFontPointSize
                        }

                        Flow {
                            anchors.topMargin:  ScreenTools.defaultFontPixelWidth
                            anchors.top:        orientationCalAreaHelpText.bottom
                            anchors.bottom:     parent.bottom
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            spacing:            ScreenTools.defaultFontPixelWidth / 2

                            property real indicatorWidth:   (width / 3) - (spacing * 2)
                            property real indicatorHeight:  (height / 2) - spacing

                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalDownSideVisible
                                calValid:           controller.orientationCalDownSideDone
                                calInProgress:      controller.orientationCalDownSideInProgress
                                calInProgressText:  controller.orientationCalDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalDownSideRotate ? "qrc:///qmlimages/VehicleDownRotate.png" : "qrc:///qmlimages/VehicleDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalUpsideDownSideVisible
                                calValid:           controller.orientationCalUpsideDownSideDone
                                calInProgress:      controller.orientationCalUpsideDownSideInProgress
                                calInProgressText:  controller.orientationCalUpsideDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalUpsideDownSideRotate ? "qrc:///qmlimages/VehicleUpsideDownRotate.png" : "qrc:///qmlimages/VehicleUpsideDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalNoseDownSideVisible
                                calValid:           controller.orientationCalNoseDownSideDone
                                calInProgress:      controller.orientationCalNoseDownSideInProgress
                                calInProgressText:  controller.orientationCalNoseDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalNoseDownSideRotate ? "qrc:///qmlimages/VehicleNoseDownRotate.png" : "qrc:///qmlimages/VehicleNoseDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalTailDownSideVisible
                                calValid:           controller.orientationCalTailDownSideDone
                                calInProgress:      controller.orientationCalTailDownSideInProgress
                                calInProgressText:  controller.orientationCalTailDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalTailDownSideRotate ? "qrc:///qmlimages/VehicleTailDownRotate.png" : "qrc:///qmlimages/VehicleTailDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalLeftSideVisible
                                calValid:           controller.orientationCalLeftSideDone
                                calInProgress:      controller.orientationCalLeftSideInProgress
                                calInProgressText:  controller.orientationCalLeftSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalLeftSideRotate ? "qrc:///qmlimages/VehicleLeftRotate.png" : "qrc:///qmlimages/VehicleLeft.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalRightSideVisible
                                calValid:           controller.orientationCalRightSideDone
                                calInProgress:      controller.orientationCalRightSideInProgress
                                calInProgressText:  controller.orientationCalRightSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        controller.orientationCalRightSideRotate ? "qrc:///qmlimages/VehicleRightRotate.png" : "qrc:///qmlimages/VehicleRight.png"
                            }
                        }
                    }
                }
            }
        } // Row
    } // Component
} // SetupPage
