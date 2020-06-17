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
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ArduPilot     1.0

SetupPage {
    id:             sensorsPage
    pageComponent:  sensorsPageComponent

    Component {
        id:             sensorsPageComponent

        Item {
            width:  availableWidth
            height: availableHeight

            // Help text which is shown both in the status text area prior to pressing a cal button and in the
            // pre-calibration dialog.

            readonly property string orientationHelpSet:    qsTr("If mounted in the direction of flight, select None.")
            readonly property string orientationHelpCal:    qsTr("Before calibrating make sure rotation settings are correct. ") + orientationHelpSet
            readonly property string compassRotationText:   qsTr("If the compass or GPS module is mounted in flight direction, leave the default value (None)")

            readonly property string compassHelp:   qsTr("For Compass calibration you will need to rotate your vehicle through a number of positions.")
            readonly property string gyroHelp:      qsTr("For Gyroscope calibration you will need to place your vehicle on a surface and leave it still.")
            readonly property string accelHelp:     qsTr("For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds.")
            readonly property string levelHelp:     qsTr("To level the horizon you need to place the vehicle in its level flight position and press OK.")

            readonly property string statusTextAreaDefaultText: qsTr("Start the individual calibration steps by clicking one of the buttons to the left.")

            // Used to pass help text to the preCalibrationDialog dialog
            property string preCalibrationDialogHelp

            property string _postCalibrationDialogText
            property var    _postCalibrationDialogParams

            readonly property string _badCompassCalText: qsTr("The calibration for Compass %1 appears to be poor. ") +
                                                         qsTr("Check the compass position within your vehicle and re-do the calibration.")

            readonly property int sideBarH1PointSize:  ScreenTools.mediumFontPointSize
            readonly property int mainTextH1PointSize: ScreenTools.mediumFontPointSize // Seems to be unused

            readonly property int rotationColumnWidth: 250

            property Fact noFact: Fact { }

            property bool accelCalNeeded:                   controller.accelSetupNeeded
            property bool compassCalNeeded:                 controller.compassSetupNeeded

            property Fact boardRot:                         controller.getParameterFact(-1, "AHRS_ORIENTATION")

            readonly property int _calTypeCompass:  1   ///< Calibrate compass
            readonly property int _calTypeAccel:    2   ///< Calibrate accel
            readonly property int _calTypeSet:      3   ///< Set orientations only
            readonly property int _buttonWidth:     ScreenTools.defaultFontPixelWidth * 15

            property bool   _orientationsDialogShowCompass: true
            property string _orientationDialogHelp:         orientationHelpSet
            property int    _orientationDialogCalType
            property real   _margins:                       ScreenTools.defaultFontPixelHeight / 2
            property bool   _compassAutoRotAvailable:       controller.parameterExists(-1, "COMPASS_AUTO_ROT")
            property Fact   _compassAutoRotFact:            controller.getParameterFact(-1, "COMPASS_AUTO_ROT", false /* reportMissing */)
            property bool   _compassAutoRot:                _compassAutoRotAvailable ? _compassAutoRotFact.rawValue == 2 : false

            function showOrientationsDialog(calType) {
                var dialogTitle
                var buttons = StandardButton.Ok

                _orientationDialogCalType = calType
                switch (calType) {
                case _calTypeCompass:
                    _orientationsDialogShowCompass = true
                    _orientationDialogHelp = orientationHelpCal
                    dialogTitle = qsTr("Calibrate Compass")
                    buttons |= StandardButton.Cancel
                    break
                case _calTypeAccel:
                    _orientationsDialogShowCompass = false
                    _orientationDialogHelp = orientationHelpCal
                    dialogTitle = qsTr("Calibrate Accelerometer")
                    buttons |= StandardButton.Cancel
                    break
                case _calTypeSet:
                    _orientationsDialogShowCompass = true
                    _orientationDialogHelp = orientationHelpSet
                    dialogTitle = qsTr("Sensor Settings")
                    break
                }

                mainWindow.showComponentDialog(orientationsDialogComponent, dialogTitle, mainWindow.showDialogDefaultWidth, buttons)
            }

            APMSensorParams {
                id:                     sensorParams
                factPanelController:    controller
            }

            APMSensorsComponentController {
                id:                         controller
                statusLog:                  statusTextArea
                progressBar:                progressBar
                nextButton:                 nextButton
                cancelButton:               cancelButton
                orientationCalAreaHelpText: orientationCalAreaHelpText

                property var rgCompassCalFitness: [ controller.compass1CalFitness, controller.compass2CalFitness, controller.compass3CalFitness ]

                onResetStatusTextArea: statusLog.text = statusTextAreaDefaultText

                onWaitingForCancelChanged: {
                    if (controller.waitingForCancel) {
                        mainWindow.showComponentDialog(waitForCancelDialogComponent, qsTr("Calibration Cancel"), mainWindow.showDialogDefaultWidth, 0)
                    }
                }

                onCalibrationComplete: {
                    switch (calType) {
                    case APMSensorsComponentController.CalTypeAccel:
                        mainWindow.showComponentDialog(postCalibrationComponent, qsTr("Accelerometer calibration complete"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
                        break
                    case APMSensorsComponentController.CalTypeOffboardCompass:
                        mainWindow.showComponentDialog(postCalibrationComponent, qsTr("Compass calibration complete"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
                        break
                    case APMSensorsComponentController.CalTypeOnboardCompass:
                        mainWindow.showComponentDialog(postOnboardCompassCalibrationComponent, qsTr("Calibration complete"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
                        break
                    }
                }

                onSetAllCalButtonsEnabled: {
                    buttonColumn.enabled = enabled
                }
            }

            Component.onCompleted: {
                var usingUDP = controller.usingUDPLink()
                var isSub = QGroundControl.multiVehicleManager.activeVehicle.sub;
                if (usingUDP && !isSub) {
                    mainWindow.showMessageDialog(qsTr("Sensor Calibration"), qsTr("Performing sensor calibration over a WiFi connection can be unreliable. If you run into problems try using a direct USB connection instead."))
                }
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

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
                id: singleCompassOnboardResultsComponent

                Column {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        Math.round(ScreenTools.defaultFontPixelHeight / 2)
                    visible:        sensorParams.rgCompassAvailable[index] && sensorParams.rgCompassUseFact[index].value

                    property real greenMaxThreshold:   8 * (sensorParams.rgCompassExternal[index] ? 1 : 2)
                    property real yellowMaxThreshold:  15 * (sensorParams.rgCompassExternal[index] ? 1 : 2)
                    property real fitnessRange:        25 * (sensorParams.rgCompassExternal[index] ? 1 : 2)

                    Item {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         ScreenTools.defaultFontPixelHeight

                        Row {
                            id:             fitnessRow
                            anchors.fill:   parent

                            Rectangle {
                                width:  parent.width * (greenMaxThreshold / fitnessRange)
                                height: parent.height
                                color:  "green"
                            }
                            Rectangle {
                                width:  parent.width * ((yellowMaxThreshold - greenMaxThreshold) / fitnessRange)
                                height: parent.height
                                color:  "yellow"
                            }
                            Rectangle {
                                width:  parent.width * ((fitnessRange - yellowMaxThreshold) / fitnessRange)
                                height: parent.height
                                color:  "red"
                            }
                        }

                        Rectangle {
                            height:                 fitnessRow.height * 0.66
                            width:                  height
                            anchors.verticalCenter: fitnessRow.verticalCenter
                            x:                      (fitnessRow.width * (Math.min(Math.max(controller.rgCompassCalFitness[index], 0.0), fitnessRange) / fitnessRange)) - (width / 2)
                            radius:                 height / 2
                            color:                  "white"
                            border.color:           "black"
                        }
                    }

                    Column {
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing:            Math.round(ScreenTools.defaultFontPixelHeight / 4)

                        QGCLabel {
                            text: qsTr("Compass ") + (index+1) + " " +
                                  (sensorParams.rgCompassPrimary[index] ? qsTr("(primary") : qsTr("(secondary")) +
                                  (sensorParams.rgCompassExternalParamAvailable[index] ?
                                       (sensorParams.rgCompassExternal[index] ? qsTr(", external") : qsTr(", internal" )) :
                                       "") +
                                  ")"
                        }

                        FactCheckBox {
                            text:       qsTr("Use Compass")
                            fact:       sensorParams.rgCompassUseFact[index]
                            visible:    sensorParams.rgCompassUseParamAvailable[index] && !sensorParams.rgCompassPrimary[index]
                        }
                    }
                }
            }

            Component {
                id: postOnboardCompassCalibrationComponent

                QGCViewDialog {
                    Column {
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing:            ScreenTools.defaultFontPixelHeight

                        Repeater {
                            model:      3
                            delegate:   singleCompassOnboardResultsComponent
                        }

                        QGCLabel {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            wrapMode:       Text.WordWrap
                            text:           qsTr("Shown in the indicator bars is the quality of the calibration for each compass.\n\n") +
                                            qsTr("- Green indicates a well functioning compass.\n") +
                                            qsTr("- Yellow indicates a questionable compass or calibration.\n") +
                                            qsTr("- Red indicates a compass which should not be used.\n\n") +
                                            qsTr("YOU MUST REBOOT YOUR VEHICLE AFTER EACH CALIBRATION.")
                        }

                        QGCButton {
                            text:       qsTr("Reboot Vehicle")
                            onClicked: {
                                controller.vehicle.rebootVehicle()
                                hideDialog()
                            }
                        }
                    }
                }
            }

            Component {
                id: postCalibrationComponent

                QGCViewDialog {
                    Column {
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing:            ScreenTools.defaultFontPixelHeight

                        QGCLabel {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            wrapMode:       Text.WordWrap
                            text:           qsTr("YOU MUST REBOOT YOUR VEHICLE AFTER EACH CALIBRATION.")
                        }

                        QGCButton {
                            text:       qsTr("Reboot Vehicle")
                            onClicked: {
                                controller.vehicle.rebootVehicle()
                                hideDialog()
                            }
                       }
                    }
                }
            }

            Component {
                id: singleCompassSettingsComponent

                Column {
                    spacing: Math.round(ScreenTools.defaultFontPixelHeight / 2)
                    visible: sensorParams.rgCompassAvailable[index]

                    QGCLabel {
                        text: qsTr("Compass ") + (index+1) + " " +
                              (sensorParams.rgCompassPrimary[index] ? qsTr("(primary") :qsTr( "(secondary")) +
                              (sensorParams.rgCompassExternalParamAvailable[index] ?
                                   (sensorParams.rgCompassExternal[index] ? qsTr(", external") : qsTr(", internal") ) :
                                   "") +
                              ")"
                    }

                    Column {
                        anchors.margins:    ScreenTools.defaultFontPixelWidth * 2
                        anchors.left:       parent.left
                        spacing:            Math.round(ScreenTools.defaultFontPixelHeight / 4)

                        FactCheckBox {
                            text:       qsTr("Use Compass")
                            fact:       sensorParams.rgCompassUseFact[index]
                            visible:    sensorParams.rgCompassUseParamAvailable[index] && !sensorParams.rgCompassPrimary[index]
                        }

                        Column {
                            visible: !_compassAutoRot && sensorParams.rgCompassExternal[index] && sensorParams.rgCompassRotParamAvailable[index]

                            QGCLabel { text: qsTr("Orientation:") }

                            FactComboBox {
                                width:      rotationColumnWidth
                                indexModel: false
                                fact:       sensorParams.rgCompassRotFact[index]
                            }
                        }
                    }
                }
            }

            Component {
                id: orientationsDialogComponent

                QGCViewDialog {
                    id: orientationsDialog

                    function accept() {
                        if (_orientationDialogCalType == _calTypeAccel) {
                            controller.calibrateAccel()
                        } else if (_orientationDialogCalType == _calTypeCompass) {
                            controller.calibrateCompass()
                        }
                        orientationsDialog.hideDialog()
                    }

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
                                text:       _orientationDialogHelp
                            }

                            Column {
                                QGCLabel { text: qsTr("Autopilot Rotation:") }

                                FactComboBox {
                                    width:      rotationColumnWidth
                                    indexModel: false
                                    fact:       boardRot
                                }
                            }

                            Repeater {
                                model:      _orientationsDialogShowCompass ? 3 : 0
                                delegate:   singleCompassSettingsComponent
                            }
                        } // Column
                    } // QGCFlickable
                } // QGCViewDialog
            } // Component - orientationsDialogComponent

            Component {
                id: compassMotDialogComponent

                QGCViewDialog {
                    id: compassMotDialog

                    function accept() {
                        controller.calibrateMotorInterference()
                        compassMotDialog.hideDialog()
                    }

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
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("This is recommended for vehicles that have only an internal compass and on vehicles where there is significant interference on the compass from the motors, power wires, etc. ") +
                                                qsTr("CompassMot only works well if you have a battery current monitor because the magnetic interference is linear with current drawn. ") +
                                                qsTr("It is technically possible to set-up CompassMot using throttle but this is not recommended.")
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("Disconnect your props, flip them over and rotate them one position around the frame. ") +
                                                qsTr("In this configuration they should push the copter down into the ground when the throttle is raised.")
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("Secure the copter (perhaps with tape) so that it does not move.")
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("Turn on your transmitter and keep throttle at zero.")
                            }

                            QGCLabel {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                text:           qsTr("Click Ok to start CompassMot calibration.")
                            }
                        } // Column
                    } // QGCFlickable
                } // QGCViewDialog
            } // Component - compassMotDialogComponent

            Component {
                id: levelHorizonDialogComponent

                QGCViewDialog {
                    id: levelHorizonDialog

                    function accept() {
                        controller.levelHorizon()
                        levelHorizonDialog.hideDialog()
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           qsTr("To level the horizon you need to place the vehicle in its level flight position and press Ok.")
                    }
                } // QGCViewDialog
            } // Component - levelHorizonDialogComponent

            Component {
                id: calibratePressureDialogComponent

                QGCViewDialog {
                    id: calibratePressureDialog

                    function accept() {
                        controller.calibratePressure()
                        calibratePressureDialog.hideDialog()
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           _helpText

                        readonly property string _altText:      activeVehicle.sub ? qsTr("depth") : qsTr("altitude")
                        readonly property string _helpText:     qsTr("Pressure calibration will set the %1 to zero at the current pressure reading. %2").arg(_altText).arg(_helpTextFW)
                        readonly property string _helpTextFW:   activeVehicle.fixedWing ? qsTr("To calibrate the airspeed sensor shield it from the wind. Do not touch the sensor or obstruct any holes during the calibration.") : ""
                    }
                } // QGCViewDialog
            } // Component - calibratePressureDialogComponent

            QGCFlickable {
                id:             buttonFlickable
                anchors.left:   parent.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                width:          _buttonWidth
                contentHeight:  nextCancelColumn.y + nextCancelColumn.height + _margins

                // Calibration button column - Calibratin buttons are kept in a separate column from Next/Cancel buttons
                // so we can enable/disable them all as a group
                Column {
                    id:                 buttonColumn
                    spacing:            _margins
                    Layout.alignment:   Qt.AlignLeft | Qt.AlignTop

                    IndicatorButton {
                        width:          _buttonWidth
                        text:           qsTr("Accelerometer")
                        indicatorGreen: !accelCalNeeded

                        onClicked: showOrientationsDialog(_calTypeAccel)
                    }

                    IndicatorButton {
                        width:          _buttonWidth
                        text:           qsTr("Compass")
                        indicatorGreen: !compassCalNeeded

                        onClicked: {
                            if (controller.accelSetupNeeded) {
                                mainWindow.showMessageDialog(qsTr("Calibrate Compass"), qsTr("Accelerometer must be calibrated prior to Compass."))
                            } else {
                                showOrientationsDialog(_calTypeCompass)
                            }
                        }
                    }

                    QGCButton {
                        width:  _buttonWidth
                        text:   _levelHorizonText

                        readonly property string _levelHorizonText: qsTr("Level Horizon")

                        onClicked: {
                            if (controller.accelSetupNeeded) {
                                mainWindow.showMessageDialog(_levelHorizonText, qsTr("Accelerometer must be calibrated prior to Level Horizon."))
                            } else {
                                mainWindow.showComponentDialog(levelHorizonDialogComponent, _levelHorizonText, mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                            }
                        }
                    }

                    QGCButton {
                        width:      _buttonWidth
                        text:       _calibratePressureText
                        onClicked:  mainWindow.showComponentDialog(calibratePressureDialogComponent, _calibratePressureText, mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)

                        readonly property string _calibratePressureText: activeVehicle.fixedWing ? qsTr("Cal Baro/Airspeed") : qsTr("Calibrate Pressure")
                    }

                    QGCButton {
                        width:      _buttonWidth
                        text:       qsTr("CompassMot")
                        visible:    activeVehicle ? activeVehicle.supportsMotorInterference : false

                        onClicked:  mainWindow.showComponentDialog(compassMotDialogComponent, qsTr("CompassMot - Compass Motor Interference Calibration"), mainWindow.showDialogFullWidth, StandardButton.Cancel | StandardButton.Ok)
                    }

                    QGCButton {
                        width:      _buttonWidth
                        text:       qsTr("Sensor Settings")
                        onClicked:  showOrientationsDialog(_calTypeSet)
                    }
                } // Column - Cal Buttons

                Column {
                    id:                 nextCancelColumn
                    anchors.topMargin:  buttonColumn.spacing
                    anchors.top:        buttonColumn.bottom
                    anchors.left:       buttonColumn.left
                    spacing:            buttonColumn.spacing

                    QGCButton {
                        id:         nextButton
                        width:      _buttonWidth
                        text:       qsTr("Next")
                        enabled:    false
                        onClicked:  controller.nextClicked()
                    }

                    QGCButton {
                        id:         cancelButton
                        width:      _buttonWidth
                        text:       qsTr("Cancel")
                        enabled:    false
                        onClicked:  controller.cancelCalibration()
                    }
                }
            } // QGCFlickable - buttons

            /// Right column - cal area
            Column {
                anchors.leftMargin: _margins
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                anchors.left:       buttonFlickable.right
                anchors.right:      parent.right

                ProgressBar {
                    id:             progressBar
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                }

                Item { height: ScreenTools.defaultFontPixelHeight; width: 10 } // spacer

                Item {
                    id:     centerPanel
                    width:  parent.width
                    height: parent.height - y

                    TextArea {
                        id:             statusTextArea
                        anchors.fill:   parent
                        readOnly:       true
                        frameVisible:   false
                        text:           statusTextAreaDefaultText

                        style: TextAreaStyle {
                            textColor:          qgcPal.text
                            backgroundColor:    qgcPal.windowShade
                        }
                    }

                    Rectangle {
                        id:             orientationCalArea
                        anchors.fill:   parent
                        visible:        controller.showOrientationCalArea
                        color:          qgcPal.windowShade

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
                            spacing:            ScreenTools.defaultFontPixelWidth

                            property real indicatorWidth:   (width / 3) - (spacing * 2)
                            property real indicatorHeight:  (height / 2) - spacing

                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalDownSideVisible
                                calValid:           controller.orientationCalDownSideDone
                                calInProgress:      controller.orientationCalDownSideInProgress
                                calInProgressText:  controller.orientationCalDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalLeftSideVisible
                                calValid:           controller.orientationCalLeftSideDone
                                calInProgress:      controller.orientationCalLeftSideInProgress
                                calInProgressText:  controller.orientationCalLeftSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleLeft.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalRightSideVisible
                                calValid:           controller.orientationCalRightSideDone
                                calInProgress:      controller.orientationCalRightSideInProgress
                                calInProgressText:  controller.orientationCalRightSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleRight.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalNoseDownSideVisible
                                calValid:           controller.orientationCalNoseDownSideDone
                                calInProgress:      controller.orientationCalNoseDownSideInProgress
                                calInProgressText:  controller.orientationCalNoseDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleNoseDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalTailDownSideVisible
                                calValid:           controller.orientationCalTailDownSideDone
                                calInProgress:      controller.orientationCalTailDownSideInProgress
                                calInProgressText:  controller.orientationCalTailDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleTailDown.png"
                            }
                            VehicleRotationCal {
                                width:              parent.indicatorWidth
                                height:             parent.indicatorHeight
                                visible:            controller.orientationCalUpsideDownSideVisible
                                calValid:           controller.orientationCalUpsideDownSideDone
                                calInProgress:      controller.orientationCalUpsideDownSideInProgress
                                calInProgressText:  controller.orientationCalUpsideDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                                imageSource:        "qrc:///qmlimages/VehicleUpsideDown.png"
                            }
                        }
                    }
                } // Item - Cal display area
            } // Column - cal display
        } // Row
    } // Component - sensorsPageComponent
} // SetupPage
