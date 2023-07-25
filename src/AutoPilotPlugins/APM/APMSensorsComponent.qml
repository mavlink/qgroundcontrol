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
import QGroundControl.QGCPositionManager    1.0

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
            property bool   _showSimpleAccelCalOption:      false
            property bool   _doSimpleAccelCal:              false
            property var    _gcsPosition:                    QGroundControl.qgcPositionManger.gcsPosition
            property var    _mapPosition:                    QGroundControl.flightMapPosition

            function showOrientationsDialog(calType) {
                var dialogTitle
                var buttons = StandardButton.Ok
                _showSimpleAccelCalOption = false

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

            function showSimpleAccelCalOption() {
                _showSimpleAccelCalOption = true
            }

            function compassLabel(index) {
                var label = qsTr("Compass %1 ").arg(index+1)
                var addOpenParan = true
                var addComma = false
                if (sensorParams.compassPrimaryFactAvailable) {
                    label += sensorParams.rgCompassPrimary[index] ? qsTr("(primary") : qsTr("(secondary")
                    addComma = true
                    addOpenParan = false
                }
                if (sensorParams.rgCompassExternalParamAvailable[index]) {
                    if (addOpenParan) {
                        label += "("
                    }
                    if (addComma) {
                        label += qsTr(", ")
                    }
                    label += sensorParams.rgCompassExternal[index] ? qsTr("external") : qsTr("internal")
                }
                label += ")"
                return label
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
                        _singleCompassSettingsComponentShowPriority = true
                        mainWindow.showComponentDialog(postOnboardCompassCalibrationComponent, qsTr("Calibration complete"), mainWindow.showDialogDefaultWidth, StandardButton.Ok)
                        break
                    }
                }

                onSetAllCalButtonsEnabled: {
                    buttonColumn.enabled = enabled
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

                    property int _index: index

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

                    Loader {
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        sourceComponent:    singleCompassSettingsComponent

                        property int index: _index
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

            property bool _singleCompassSettingsComponentShowPriority: true
            Component {
                id: singleCompassSettingsComponent

                Column {
                    spacing: Math.round(ScreenTools.defaultFontPixelHeight / 2)
                    visible: sensorParams.rgCompassAvailable[index]

                    QGCLabel {
                        text: compassLabel(index)
                    }
                    APMSensorIdDecoder {
                        fact: sensorParams.rgCompassId[index]
                    }

                    Column {
                        anchors.margins:    ScreenTools.defaultFontPixelWidth * 2
                        anchors.left:       parent.left
                        spacing:            Math.round(ScreenTools.defaultFontPixelHeight / 4)

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth

                            FactCheckBox {
                                id:         useCompassCheckBox
                                text:       qsTr("Use Compass")
                                fact:       sensorParams.rgCompassUseFact[index]
                                visible:    sensorParams.rgCompassUseParamAvailable[index] && !sensorParams.rgCompassPrimary[index]
                            }

                            QGCComboBox {
                                model:      [ qsTr("Priority 1"), qsTr("Priority 2"), qsTr("Priority 3"), qsTr("Not Set") ]
                                visible:    _singleCompassSettingsComponentShowPriority && sensorParams.compassPrioFactsAvailable && useCompassCheckBox.visible && useCompassCheckBox.checked

                                property int _compassIndex: index

                                function selectPriorityfromParams() {
                                    currentIndex = 3
                                    var compassId = sensorParams.rgCompassId[_compassIndex].rawValue
                                    for (var prioIndex=0; prioIndex<3; prioIndex++) {
                                        console.log(`comparing ${compassId} with ${sensorParams.rgCompassPrio[prioIndex].rawValue} (index ${prioIndex})`)
                                        if (compassId == sensorParams.rgCompassPrio[prioIndex].rawValue) {
                                            currentIndex = prioIndex
                                            break
                                        }
                                    }
                                }

                                Component.onCompleted: selectPriorityfromParams()

                                onActivated: {
                                    if (index == 3) {
                                        // User cannot select Not Set
                                        selectPriorityfromParams()
                                    } else {
                                        sensorParams.rgCompassPrio[index].rawValue = sensorParams.rgCompassId[_compassIndex].rawValue
                                    }
                                }
                            }
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

                    function compassMask () {
                        var mask = 0
                        mask |=  (0 + (sensorParams.rgCompassPrio[0].rawValue !== 0)) << 0
                        mask |=  (0 + (sensorParams.rgCompassPrio[1].rawValue !== 0)) << 1
                        mask |=  (0 + (sensorParams.rgCompassPrio[2].rawValue !== 0)) << 2
                        return mask
                    }

                    function accept() {
                        if (_orientationDialogCalType == _calTypeAccel) {
                            controller.calibrateAccel(_doSimpleAccelCal)
                        } else if (_orientationDialogCalType == _calTypeCompass) {
                            if (!northCalibrationCheckBox.checked) {
                                controller.calibrateCompass()
                            } else {
                               var lat = parseFloat(northCalLat.text)
                               var lon = parseFloat(northCalLon.text)
                               if (useMapPositionCheckbox.checked) {
                                   lat = _mapPosition.latitude
                                   lon = _mapPosition.longitude
                               }
                               if (useGcsPositionCheckbox.checked) {
                                   lat = _gcsPosition.latitude
                                   lon = _gcsPosition.longitude
                               }
                               if (isNaN(lat) || isNaN(lon)) {
                                   return
                               }
                               controller.calibrateCompassNorth(lat, lon, compassMask())
                            }
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

                            Column {

                                visible: _orientationDialogCalType == _calTypeAccel
                                spacing: ScreenTools.defaultFontPixelHeight

                                QGCLabel {
                                    width:      parent.width
                                    wrapMode:   Text.WordWrap
                                    text: qsTr("Simple accelerometer calibration is less precise but allows calibrating without rotating the vehicle. Check this if you have a large/heavy vehicle.")
                                }

                                QGCCheckBox {
                                    text: "Simple Accelerometer Calibration"
                                    onClicked: _doSimpleAccelCal = this.checked
                                }
                            }

                            Repeater {
                                model:      _orientationsDialogShowCompass ? 3 : 0
                                delegate:   singleCompassSettingsComponent
                            }

                            QGCLabel {
                                id:         magneticDeclinationLabel
                                width:      parent.width
                                visible:    globals.activeVehicle.sub && _orientationsDialogShowCompass
                                text:       qsTr("Magnetic Declination")
                            }

                            Column {
                                visible:            magneticDeclinationLabel.visible
                                anchors.margins:    ScreenTools.defaultFontPixelWidth
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                spacing:            ScreenTools.defaultFontPixelHeight

                                QGCCheckBox {
                                    id:                           manualMagneticDeclinationCheckBox
                                    text:                         qsTr("Manual Magnetic Declination")
                                    property Fact autoDecFact:    controller.getParameterFact(-1, "COMPASS_AUTODEC")
                                    property int manual:          0
                                    property int automatic:       1

                                    checked:    autoDecFact.rawValue === manual
                                    onClicked:  autoDecFact.value = (checked ? manual : automatic)
                                }

                                FactTextField {
                                    fact:       sensorParams.declinationFact
                                    enabled:    manualMagneticDeclinationCheckBox.checked
                                }
                            }

                            Item { height: ScreenTools.defaultFontPixelHeight; width: 10 } // spacer

                            QGCLabel {
                                id:         northCalibrationLabel
                                width:      parent.width
                                visible:    _orientationsDialogShowCompass
                                wrapMode:   Text.WordWrap
                                text:       qsTr("Fast compass calibration given vehicle position and yaw. This ") +
                                            qsTr("results in zero diagonal and off-diagonal elements, so is only ") +
                                            qsTr("suitable for vehicles where the field is close to spherical. It is ") +
                                            qsTr("useful for large vehicles where moving the vehicle to calibrate it ") +
                                            qsTr("is difficult. Point the vehicle North before using it.")
                            }

                            Column {
                                visible:            northCalibrationLabel.visible
                                anchors.margins:    ScreenTools.defaultFontPixelWidth
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                spacing:            ScreenTools.defaultFontPixelHeight

                                QGCCheckBox {
                                    id:             northCalibrationCheckBox
                                    visible:        northCalibrationLabel.visible
                                    text:           qsTr("Fast Calibration")
                                }

                                QGCLabel {
                                    id:         northCalibrationManualPosition
                                    width:      parent.width
                                    visible:    northCalibrationCheckBox.checked && !globals.activeVehicle.coordinate.isValid
                                    wrapMode:   Text.WordWrap
                                    text:       qsTr("Vehicle has no Valid positon, please provide it")
                                }

                                QGCCheckBox {
                                    visible:    northCalibrationManualPosition.visible && _gcsPosition.isValid
                                    id:         useGcsPositionCheckbox
                                    text:       qsTr("Use GCS position instead")
                                    checked:    _gcsPosition.isValid
                                }
                                QGCCheckBox {
                                    visible:    northCalibrationManualPosition.visible && !_gcsPosition.isValid
                                    id:         useMapPositionCheckbox
                                    text:       qsTr("Use current map position instead")
                                }

                                QGCLabel {
                                    width:      parent.width
                                    visible:    useMapPositionCheckbox.checked
                                    wrapMode:   Text.WordWrap
                                    text:       qsTr(`Lat: ${_mapPosition.latitude.toFixed(4)} Lon: ${_mapPosition.longitude.toFixed(4)}`)
                                }

                                FactTextField {
                                    id:         northCalLat
                                    visible:    !useGcsPositionCheckbox.checked && !useMapPositionCheckbox.checked && northCalibrationCheckBox.checked
                                    text:       "0.00"
                                    textColor:  isNaN(parseFloat(text)) ? qgcPal.warningText: qgcPal.textFieldText
                                    enabled:    !useGcsPositionCheckbox.checked
                                }
                                FactTextField {
                                    id:         northCalLon
                                    visible:    !useGcsPositionCheckbox.checked && !useMapPositionCheckbox.checked && northCalibrationCheckBox.checked
                                    text:       "0.00"
                                    textColor:  isNaN(parseFloat(text)) ? qgcPal.warningText: qgcPal.textFieldText
                                    enabled:    !useGcsPositionCheckbox.checked
                                }

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

                        readonly property string _altText:      globals.activeVehicle.sub ? qsTr("depth") : qsTr("altitude")
                        readonly property string _helpText:     qsTr("Pressure calibration will set the %1 to zero at the current pressure reading. %2").arg(_altText).arg(_helpTextFW)
                        readonly property string _helpTextFW:   globals.activeVehicle.fixedWing ? qsTr("To calibrate the airspeed sensor shield it from the wind. Do not touch the sensor or obstruct any holes during the calibration.") : ""
                    }
                } // QGCViewDialog
            } // Component - calibratePressureDialogComponent

            Component {
                id: calibrateGyroDialogComponent

                QGCViewDialog {
                    id: calibrateGyroDialog

                    function accept() {
                        controller.calibrateGyro()
                        calibrateGyroDialog.hideDialog()
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           qsTr("For Gyroscope calibration you will need to place your vehicle on a surface and leave it still.\n\nClick Ok to start calibration.")
                    }
                }
            }

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

                        onClicked: function () {
                            showOrientationsDialog(_calTypeAccel);
                            showSimpleAccelCalOption();
                        }
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
                        text:       qsTr("Gyro")
                        visible:    globals.activeVehicle && (globals.activeVehicle.multiRotor | globals.activeVehicle.rover | globals.activeVehicle.sub)
                        onClicked:  mainWindow.showComponentDialog(calibrateGyroDialogComponent, qsTr("Calibrate Gyro"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
                    }

                    QGCButton {
                        width:      _buttonWidth
                        text:       _calibratePressureText
                        onClicked:  mainWindow.showComponentDialog(calibratePressureDialogComponent, _calibratePressureText, mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)

                        readonly property string _calibratePressureText: globals.activeVehicle.fixedWing ? qsTr("Baro/Airspeed") : qsTr("Pressure")
                    }

                    QGCButton {
                        width:      _buttonWidth
                        text:       qsTr("CompassMot")
                        visible:    globals.activeVehicle ? globals.activeVehicle.supportsMotorInterference : false

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
