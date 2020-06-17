/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

/// Page for sensor calibration. This control is used within the SensorsComponent control and can also be used
/// standalone for custom uis. When using standardalone you can use the various show* bools to show/hide what you want.
Item {
    id: _root

    property bool   showSensorCalibrationCompass:   true    ///< true: Show this calibration button
    property bool   showSensorCalibrationGyro:      true    ///< true: Show this calibration button
    property bool   showSensorCalibrationAccel:     true    ///< true: Show this calibration button
    property bool   showSensorCalibrationLevel:     true    ///< true: Show this calibration button
    property bool   showSensorCalibrationAirspeed:  true    ///< true: Show this calibration button
    property bool   showSetOrientations:            true    ///< true: Show this calibration button
    property bool   showNextButton:                 false   ///< true: Show Next button which will signal nextButtonClicked

    signal nextButtonClicked

    // Help text which is shown both in the status text area prior to pressing a cal button and in the
    // pre-calibration dialog.

    readonly property string boardRotationText: qsTr("If the orientation is in the direction of flight, select ROTATION_NONE.")
    readonly property string compassRotationText: qsTr("If the orientation is in the direction of flight, select ROTATION_NONE.")

    readonly property string compassHelp:   qsTr("For Compass calibration you will need to rotate your vehicle through a number of positions.\n\nClick Ok to start calibration.")
    readonly property string gyroHelp:      qsTr("For Gyroscope calibration you will need to place your vehicle on a surface and leave it still.\n\nClick Ok to start calibration.")
    readonly property string accelHelp:     qsTr("For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds.\n\nClick Ok to start calibration.")
    readonly property string levelHelp:     qsTr("To level the horizon you need to place the vehicle in its level flight position and press OK.")
    readonly property string airspeedHelp:  qsTr("For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor. Do not touch the sensor or obstruct any holes during the calibration.")

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

    property bool   _sensorsHaveFixedOrientation:   QGroundControl.corePlugin.options.sensorsHaveFixedOrientation
    property bool   _wifiReliableForCalibration:    QGroundControl.corePlugin.options.wifiReliableForCalibration
    property int    _buttonWidth:                   ScreenTools.defaultFontPixelWidth * 15

    SensorsComponentController {
        id:                         controller
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

        onMagCalComplete: {
            setOrientationsDialogShowBoardOrientation = false
            mainWindow.showComponentDialog(setOrientationsDialogComponent, qsTr("Compass Calibration Complete"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
        }

        onWaitingForCancelChanged: {
            if (controller.waitingForCancel) {
                mainWindow.showComponentDialog(waitForCancelDialogComponent, qsTr("Calibration Cancel"), mainWindow.showDialogDefaultWidth, 0)
            }
        }
    }

    Component.onCompleted: {
        var usingUDP = controller.usingUDPLink()
        if (usingUDP && !_wifiReliableForCalibration) {
            mainWindow.showMessageDialog(qsTr("Sensor Calibration"), qsTr("Performing sensor calibration over a WiFi connection is known to be unreliable. You should disconnect and perform calibration using a direct USB connection instead."))
        }
    }

    Component {
        id: waitForCancelDialogComponent

        QGCViewMessage {
            message: qsTr("Waiting for Vehicle to response to Cancel. This may take a few seconds.")

            Connections {
                target: controller

                onWaitingForCancelChanged: {
                    if (!controller.waitingForCancel) {
                        hideDialog()
                    }
                }
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
                anchors.fill:   parent
                spacing:        ScreenTools.defaultFontPixelHeight

                Column {
                    width:          parent.width
                    spacing:        5
                    visible:        !_sensorsHaveFixedOrientation

                    QGCLabel {
                        id:         boardRotationHelp
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        visible:    (preCalibrationDialogType != "airspeed") && (preCalibrationDialogType != "gyro")
                        text:       qsTr("Set autopilot orientation before calibrating.")
                    }

                    Column {
                        visible:    boardRotationHelp.visible

                        QGCLabel { text: qsTr("Autopilot Orientation:") }

                        FactComboBox {
                            id:     boardRotationCombo
                            width:  rotationColumnWidth;
                            model:  rotations
                            fact:   sens_board_rot
                        }
                    }
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       preCalibrationDialogHelp
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
                        text:       _sensorsHaveFixedOrientation ?
                                        qsTr("Make sure to reboot the vehicle prior to flight.") :
                                        qsTr("Set your compass orientations below and the make sure to reboot the vehicle prior to flight.")
                    }

                    QGCButton {
                        text: qsTr("Reboot Vehicle")
                        onClicked: {
                            controller.vehicle.rebootVehicle()
                            hideDialog()
                        }
                    }

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        text:       boardRotationText
                        visible:    !_sensorsHaveFixedOrientation
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

                    // Compass 0 rotation
                    Column {
                        visible: !_sensorsHaveFixedOrientation

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

                    // Compass 1 rotation
                    Column {
                        visible: !_sensorsHaveFixedOrientation

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

                    // Compass 2 rotation
                    Column {
                        visible: !_sensorsHaveFixedOrientation
                        spacing: ScreenTools.defaultFontPixelWidth

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

    QGCFlickable {
        id:             buttonFlickable
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        width:          _buttonWidth
        contentHeight:  buttonColumn.height + buttonColumn.spacing

        Column {
            id:         buttonColumn
            spacing:    ScreenTools.defaultFontPixelHeight / 2

            IndicatorButton {
                property bool 	_hasMag: controller.parameterExists(-1, "SYS_HAS_MAG") ? controller.getParameterFact(-1, "SYS_HAS_MAG").value !== 0 : true
                id:             compassButton
                width:          _buttonWidth
                text:           qsTr("Compass")
                indicatorGreen: cal_mag0_id.value !== 0
                visible:        _hasMag && QGroundControl.corePlugin.options.showSensorCalibrationCompass && showSensorCalibrationCompass

                onClicked: {
                    preCalibrationDialogType = "compass"
                    preCalibrationDialogHelp = compassHelp
                    mainWindow.showComponentDialog(preCalibrationDialogComponent, qsTr("Calibrate Compass"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }

            IndicatorButton {
                id:             gyroButton
                width:          _buttonWidth
                text:           qsTr("Gyroscope")
                indicatorGreen: cal_gyro0_id.value !== 0
                visible:        QGroundControl.corePlugin.options.showSensorCalibrationGyro && showSensorCalibrationGyro

                onClicked: {
                    preCalibrationDialogType = "gyro"
                    preCalibrationDialogHelp = gyroHelp
                    mainWindow.showComponentDialog(preCalibrationDialogComponent, qsTr("Calibrate Gyro"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }

            IndicatorButton {
                id:             accelButton
                width:          _buttonWidth
                text:           qsTr("Accelerometer")
                indicatorGreen: cal_acc0_id.value !== 0
                visible:        QGroundControl.corePlugin.options.showSensorCalibrationAccel && showSensorCalibrationAccel

                onClicked: {
                    preCalibrationDialogType = "accel"
                    preCalibrationDialogHelp = accelHelp
                    mainWindow.showComponentDialog(preCalibrationDialogComponent, qsTr("Calibrate Accelerometer"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }

            IndicatorButton {
                id:             levelButton
                width:          _buttonWidth
                text:           qsTr("Level Horizon")
                indicatorGreen: sens_board_x_off.value !== 0 || sens_board_y_off.value !== 0 | sens_board_z_off.value !== 0
                enabled:        cal_acc0_id.value !== 0 && cal_gyro0_id.value !== 0
                visible:        QGroundControl.corePlugin.options.showSensorCalibrationLevel && showSensorCalibrationLevel

                onClicked: {
                    preCalibrationDialogType = "level"
                    preCalibrationDialogHelp = levelHelp
                    mainWindow.showComponentDialog(preCalibrationDialogComponent, qsTr("Level Horizon"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }

            IndicatorButton {
                id:             airspeedButton
                width:          _buttonWidth
                text:           qsTr("Airspeed")
                visible:        (controller.vehicle.fixedWing || controller.vehicle.vtol) &&
                                controller.getParameterFact(-1, "FW_ARSP_MODE").value == 0 &&
                                controller.getParameterFact(-1, "CBRK_AIRSPD_CHK").value !== 162128 &&
                                QGroundControl.corePlugin.options.showSensorCalibrationAirspeed &&
                                showSensorCalibrationAirspeed
                indicatorGreen: sens_dpres_off.value !== 0

                onClicked: {
                    preCalibrationDialogType = "airspeed"
                    preCalibrationDialogHelp = airspeedHelp
                    mainWindow.showComponentDialog(preCalibrationDialogComponent, qsTr("Calibrate Airspeed"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                }
            }

            QGCButton {
                id:         cancelButton
                width:      _buttonWidth
                text:       qsTr("Cancel")
                enabled:    false
                onClicked:  controller.cancelCalibration()
            }


            QGCButton {
                id:         nextButton
                width:      _buttonWidth
                text:       qsTr("Next")
                visible:    showNextButton
                onClicked:  _root.nextButtonClicked()
            }

            QGCButton {
                id:         setOrientationsButton
                width:      _buttonWidth
                text:       qsTr("Set Orientations")
                visible:    !_sensorsHaveFixedOrientation && showSetOrientations

                onClicked:  {
                    setOrientationsDialogShowBoardOrientation = true
                    mainWindow.showComponentDialog(setOrientationsDialogComponent, qsTr("Set Orientations"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
                }
            }
        } // Column - Buttons
    } // QGCFLickable - Buttons

    Column {
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.left:       buttonFlickable.right
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
}
