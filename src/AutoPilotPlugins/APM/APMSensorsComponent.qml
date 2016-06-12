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

QGCView {
    id:         qgcView
    viewPanel:  panel

    // Help text which is shown both in the status text area prior to pressing a cal button and in the
    // pre-calibration dialog.

    readonly property string orientationHelpSet:    "If the orientation is in the direction of flight, select None."
    readonly property string orientationHelpCal:    "Before calibrating make sure orientation settings are correct. " + orientationHelpSet
    readonly property string compassRotationText:   "If the compass or GPS module is mounted in flight direction, leave the default value (None)"

    readonly property string compassHelp:   "For Compass calibration you will need to rotate your vehicle through a number of positions."
    readonly property string gyroHelp:      "For Gyroscope calibration you will need to place your vehicle on a surface and leave it still."
    readonly property string accelHelp:     "For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds."
    readonly property string levelHelp:     "To level the horizon you need to place the vehicle in its level flight position and press OK."
    readonly property string airspeedHelp:  "For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor."

    readonly property string statusTextAreaDefaultText: "Start the individual calibration steps by clicking one of the buttons to the left."

    // Used to pass help text to the preCalibrationDialog dialog
    property string preCalibrationDialogHelp

    property string _postCalibrationDialogText
    property var    _postCalibrationDialogParams

    readonly property string _badCompassCalText: "The calibration for Compass %1 appears to be poor. " +
                                                 "Check the compass position within your vehicle and re-do the calibration."

    readonly property int sideBarH1PointSize:  ScreenTools.mediumFontPointSize
    readonly property int mainTextH1PointSize: ScreenTools.mediumFontPointSize // Seems to be unused

    readonly property int rotationColumnWidth: 250

    property Fact compass1Id:           controller.getParameterFact(-1, "COMPASS_DEV_ID")
    property Fact compass2Id:           controller.getParameterFact(-1, "COMPASS_DEV_ID2")
    property Fact compass3Id:           controller.getParameterFact(-1, "COMPASS_DEV_ID3")
    property Fact compass1ExternalFact: controller.getParameterFact(-1, "COMPASS_EXTERNAL")
    property Fact compass1Rot:          controller.getParameterFact(-1, "COMPASS_ORIENT")

    property Fact boardRot:             controller.getParameterFact(-1, "AHRS_ORIENTATION")

    property bool accelCalNeeded:       controller.accelSetupNeeded
    property bool compassCalNeeded:     controller.compassSetupNeeded


    // The following parameters are not available in olders firmwares

    property bool compass2ExternalParamAvailable:   controller.parameterExists(-1, "COMPASS_EXTERN2")
    property bool compass3ExternalParamAvailable:   controller.parameterExists(-1, "COMPASS_EXTERN3")
    property bool compass2RotParamAvailable:        controller.parameterExists(-1, "COMPASS_ORIENT2")
    property bool compass3RotParamAvailable:        controller.parameterExists(-1, "COMPASS_ORIENT3")
    property bool compass1UseParamAvailable:        controller.parameterExists(-1, "COMPASS_USE")
    property bool compass2UseParamAvailable:        controller.parameterExists(-1, "COMPASS_USE2")
    property bool compass3UseParamAvailable:        controller.parameterExists(-1, "COMPASS_USE3")

    property Fact noFact: Fact { }
    property Fact compass2ExternalFact: compass2ExternalParamAvailable ? controller.getParameterFact(-1, "COMPASS_EXTERN2") : noFact
    property Fact compass3ExternalFact: compass3ExternalParamAvailable ? controller.getParameterFact(-1, "COMPASS_EXTERN3") : noFact
    property Fact compass2Rot:          compass2RotParamAvailable ? controller.getParameterFact(-1, "COMPASS_ORIENT2") : noFact
    property Fact compass3Rot:          compass3RotParamAvailable ? controller.getParameterFact(-1, "COMPASS_ORIENT3") : noFact
    property Fact compass1UseFact:      compass1UseParamAvailable ? controller.getParameterFact(-1, "COMPASS_USE") : noFact
    property Fact compass2UseFact:      compass2UseParamAvailable ? controller.getParameterFact(-1, "COMPASS_USE2") : noFact
    property Fact compass3UseFact:      compass3UseParamAvailable ? controller.getParameterFact(-1, "COMPASS_USE3") : noFact

    // We track these values by binding through a separate property so we can handle missing params
    property bool compass1External: compass1ExternalFact.value
    property bool compass2External: compass2ExternalParamAvailable ? compass2ExternalFact.value : false // false: Simulate internal so we don't show rotation combos
    property bool compass3External: compass3ExternalParamAvailable ? compass3ExternalFact.value : false // false: Simulate internal so we don't show rotation combos
    property bool compass1Use:      compass1UseParamAvailable ? compass1UseFact.value : true
    property bool compass2Use:      compass2UseParamAvailable ? compass2UseFact.value : true
    property bool compass3Use:      compass3UseParamAvailable ? compass3UseFact.value : true

    // Id > = signals compass available, rot < 0 signals internal compass
    property bool showCompass1Rot: compass1Id.value > 0 && compass1External && compass1Use
    property bool showCompass2Rot: compass2Id.value > 0 && compass2External && compass2Use
    property bool showCompass3Rot: compass3Id.value > 0 && compass3External && compass3Use

    readonly property int _calTypeCompass:  1   ///< Calibrate compass
    readonly property int _calTypeAccel:    2   ///< Calibrate accel
    readonly property int _calTypeSet:      3   ///< Set orientations only

    property bool   _orientationsDialogShowCompass: true
    property string _orientationDialogHelp:         orientationHelpSet
    property int    _orientationDialogCalType

    function validCompassOffsets(compassParamPrefix) {
        var ofsX = controller.getParameterFact(-1, compassParamPrefix + "X")
        var ofsY = controller.getParameterFact(-1, compassParamPrefix + "Y")
        var ofsZ = controller.getParameterFact(-1, compassParamPrefix + "Z")
        return Math.sqrt(ofsX.value^2 + ofsY.value^2 + ofsZ.value^2) < 600
    }

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
            dialogTitle = qsTr("SetOrientations")
            break
        }

        showDialog(orientationsDialogComponent, dialogTitle, qgcView.showDialogDefaultWidth, buttons)
    }

    APMSensorsComponentController {
        id:                         controller
        factPanel:                  panel
        statusLog:                  statusTextArea
        progressBar:                progressBar
        compassButton:              compassButton
        accelButton:                accelButton
        nextButton:                 nextButton
        cancelButton:               cancelButton
        setOrientationsButton:      setOrientationsButton
        orientationCalAreaHelpText: orientationCalAreaHelpText

        onResetStatusTextArea: statusLog.text = statusTextAreaDefaultText

        onWaitingForCancelChanged: {
            if (controller.waitingForCancel) {
                showMessage(qsTr("Calibration Cancel"), qsTr("Waiting for Vehicle to response to Cancel. This may take a few seconds."), 0)
            } else {
                hideDialog()
            }
        }

        onCalibrationComplete: {
            if (_orientationDialogCalType == _calTypeAccel) {
                _postCalibrationDialogText = qsTr("Accelerometer calibration complete.")
                _postCalibrationDialogParams = [ "INS_ACCSCAL_X", "INS_ACCSCAL_Y", "INS_ACCSCAL_Z",
                                                "INS_ACC2SCAL_X", "INS_ACC2SCAL_Y", "INS_ACC2SCAL_Z",
                                                "INS_ACC3SCAL_X", "INS_ACC3SCAL_Y", "INS_ACC3SCAL_Z",
                                                "INS_GYROFFS_X", "INS_GYROFFS_Y", "INS_GYROFFS_Z",
                                                "INS_GYR2OFFS_X", "INS_GYR2OFFS_Y", "INS_GYR2OFFS_Z",
                                                "INS_GYR3OFFS_X", "INS_GYR3OFFS_Y", "INS_GYR3OFFS_Z" ]
            } else if (_orientationDialogCalType == _calTypeCompass) {
                _postCalibrationDialogText = qsTr("Compass calibration complete. ")
                _postCalibrationDialogParams = [];
                if (compass1Id.value > 0) {
                    if (!validCompassOffsets("COMPASS_OFS_")) {
                        _postCalibrationDialogText += _badCompassCalText.replace("%1", 1)
                    }
                    _postCalibrationDialogParams.push("COMPASS_OFS_X")
                    _postCalibrationDialogParams.push("COMPASS_OFS_Y")
                    _postCalibrationDialogParams.push("COMPASS_OFS_Z")
                }
                if (compass2Id.value > 0) {
                    if (!validCompassOffsets("COMPASS_OFS_")) {
                        _postCalibrationDialogText += _badCompassCalText.replace("%1", 2)
                    }
                    _postCalibrationDialogParams.push("COMPASS_OFS2_X")
                    _postCalibrationDialogParams.push("COMPASS_OFS2_Y")
                    _postCalibrationDialogParams.push("COMPASS_OFS2_Z")
                }
                if (compass3Id.value > 0) {
                    if (!validCompassOffsets("COMPASS_OFS_")) {
                        _postCalibrationDialogText += _badCompassCalText.replace("%1", 3)
                    }
                    _postCalibrationDialogParams.push("COMPASS_OFS3_X")
                    _postCalibrationDialogParams.push("COMPASS_OFS3_Y")
                    _postCalibrationDialogParams.push("COMPASS_OFS3_Z")
                }
            }
            showDialog(postCalibrationDialogComponent, qsTr("Calibration complete"), qgcView.showDialogDefaultWidth, StandardButton.Ok)
        }
    }

    Component.onCompleted: {
        var usingUDP = controller.usingUDPLink()
        if (usingUDP) {
            console.log("onUsingUDPLink")
            showMessage("Sensor Calibration", "Performing sensor calibration over a WiFi connection is known to be unreliable. You should disconnect and perform calibration using a direct USB connection instead.", StandardButton.Ok)
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    Component {
        id: postCalibrationDialogComponent

        QGCViewDialog {
            QGCLabel {
                id:             textLabel
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           _postCalibrationDialogText
            }

            QGCCheckBox {
                id:                 showValues
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                anchors.top:        textLabel.bottom
                text:               qsTr("Show values")
            }

            QGCFlickable {
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                anchors.top:        showValues.bottom
                anchors.bottom:     parent.bottom
                contentHeight:      valueColumn.height
                flickableDirection: Flickable.VerticalFlick
                visible:            showValues.checked

                Column {
                    id: valueColumn

                    Repeater {
                        model: _postCalibrationDialogParams

                        QGCLabel {
                            text: fact.name +": " + fact.valueString

                            property Fact fact: controller.getParameterFact(-1, modelData)
                        }
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
                        QGCLabel {
                            text: qsTr("Autopilot Orientation:")
                        }

                        FactComboBox {
                            width:      rotationColumnWidth
                            indexModel: false
                            fact:       boardRot
                        }
                    }

                    Column {
                        visible: _orientationsDialogShowCompass

                        Component {
                            id: compass1ComponentLabel

                            QGCLabel {
                                text: qsTr("Compass 1 Orientation:")
                            }
                        }

                        Component {
                            id: compass1ComponentCombo

                            FactComboBox {
                                width:      rotationColumnWidth
                                indexModel: false
                                fact:       compass1Rot
                            }
                        }

                        Loader { sourceComponent: showCompass1Rot ? compass1ComponentLabel : null }
                        Loader { sourceComponent: showCompass1Rot ? compass1ComponentCombo : null }
                    }

                    Column {
                        visible: _orientationsDialogShowCompass

                        Component {
                            id: compass2ComponentLabel

                            QGCLabel {
                                text: qsTr("Compass 2 Orientation:")
                            }
                        }

                        Component {
                            id: compass2ComponentCombo

                            FactComboBox {
                                width:      rotationColumnWidth
                                indexModel: false
                                fact:       compass2Rot
                            }
                        }

                        Loader { sourceComponent: showCompass2Rot ? compass2ComponentLabel : null }
                        Loader { sourceComponent: showCompass2Rot ? compass2ComponentCombo : null }
                    }

                    Column {
                        visible: _orientationsDialogShowCompass

                        Component {
                            id: compass3ComponentLabel

                            QGCLabel {
                                text: qsTr("Compass 3 Orientation")
                            }
                        }

                        Component {
                            id: compass3ComponentCombo

                            FactComboBox {
                                width:      rotationColumnWidth
                                indexModel: false
                                fact:       compass3Rot
                            }
                        }
                        Loader { sourceComponent: showCompass3Rot ? compass3ComponentLabel : null }
                        Loader { sourceComponent: showCompass3Rot ? compass3ComponentCombo : null }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - orientationsDialogComponent

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Column {
            id:         buttonColumn
            spacing:    ScreenTools.defaultFontPixelHeight / 2

            readonly property int buttonWidth: ScreenTools.defaultFontPixelWidth * 15

            IndicatorButton {
                id:             accelButton
                width:          parent.buttonWidth
                text:           qsTr("Accelerometer")
                indicatorGreen: !accelCalNeeded

                onClicked: showOrientationsDialog(_calTypeAccel)
            }

            IndicatorButton {
                id:             compassButton
                width:          parent.buttonWidth
                text:           qsTr("Compass")
                indicatorGreen: !compassCalNeeded

                onClicked: {
                    if (controller.accelSetupNeeded) {
                        showMessage(qsTr("Calibrate Compass"), qsTr("Accelerometer must be calibrated prior to Compass."), StandardButton.Ok)
                    } else {
                        showOrientationsDialog(_calTypeCompass)
                    }
                }
            }

            QGCButton {
                id:         nextButton
                width:      parent.buttonWidth
                text:       qsTr("Next")
                enabled:    false
                onClicked:  controller.nextClicked()
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
                onClicked:  showOrientationsDialog(_calTypeSet)
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
            } // Item - Cal display area
        } // Column - cal display
    } // QGCViewPanel
} // QGCView
