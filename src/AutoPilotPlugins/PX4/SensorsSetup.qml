import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

/// Page for sensor calibration. This control is used within the SensorsComponent control and can also be used
/// standalone for custom uis. When using standadalone you can use the various show* bools to show/hide what you want.
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

    readonly property string compassHelp:   qsTr("For Compass calibration you will need to rotate your vehicle through a number of positions.")
    readonly property string gyroHelp:      qsTr("For Gyroscope calibration you will need to place your vehicle on a surface and leave it still.")
    readonly property string accelHelp:     qsTr("For Accelerometer calibration you will need to place your vehicle on all six sides on a perfectly level surface and hold it still in each orientation for a few seconds.")
    readonly property string levelHelp:     qsTr("To level the horizon you need to place the vehicle in its level flight position and leave still.")
    readonly property string airspeedHelp:  qsTr("For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor. Do not touch the sensor or obstruct any holes during the calibration.")

    readonly property string statusTextAreaDefaultText: qsTr("Start the individual calibration steps by clicking one of the buttons to the left.")

    // Used to pass what type of calibration is being performed to the preCalibrationDialog
    property string preCalibrationDialogType

    // Used to pass help text to the preCalibrationDialog dialog
    property string preCalibrationDialogHelp

    property Fact cal_mag0_id:      controller.getParameterFact(-1, "CAL_MAG0_ID")
    property Fact cal_mag1_id:      controller.getParameterFact(-1, "CAL_MAG1_ID")
    property Fact cal_mag2_id:      controller.getParameterFact(-1, "CAL_MAG2_ID")
    property Fact cal_mag0_rot:     controller.getParameterFact(-1, "CAL_MAG0_ROT")
    property Fact cal_mag1_rot:     controller.getParameterFact(-1, "CAL_MAG1_ROT")
    property Fact cal_mag2_rot:     controller.getParameterFact(-1, "CAL_MAG2_ROT")

    property Fact cal_gyro0_id:     controller.getParameterFact(-1, "CAL_GYRO0_ID")
    property Fact cal_acc0_id:      controller.getParameterFact(-1, "CAL_ACC0_ID")

    property Fact sens_board_rot:   controller.getParameterFact(-1, "SENS_BOARD_ROT")
    property Fact sens_dpres_off:   controller.getParameterFact(-1, "SENS_DPRES_OFF")

    // Id > = signals compass available, rot < 0 signals internal compass
    property bool showCompass0Rot: cal_mag0_id.value > 0 && cal_mag0_rot.value >= 0
    property bool showCompass1Rot: cal_mag1_id.value > 0 && cal_mag1_rot.value >= 0
    property bool showCompass2Rot: cal_mag2_id.value > 0 && cal_mag2_rot.value >= 0

    property bool   _sensorsHaveFixedOrientation:       QGroundControl.corePlugin.options.sensorsHaveFixedOrientation
    property string _calMagIdParamFormat:               "CAL_MAG#_ID"
    property string _calMagRotParamFormat:              "CAL_MAG#_ROT"
    property bool 	_allMagsDisabled:                   controller.parameterExists(-1, "SYS_HAS_MAG") ? controller.getParameterFact(-1, "SYS_HAS_MAG").value === 0 : false
    property bool   _boardOrientationChangeAllowed:     !_sensorsHaveFixedOrientation && setOrientationsDialogShowBoardOrientation
    property bool   _compassOrientationChangeAllowed:   !_sensorsHaveFixedOrientation
    property int    _arbitrarilyLargeMaxMagIndex:       50

    function currentMagParamCount() {
        if (_allMagsDisabled) {
            return 0
        } else {
            for (var index=0; index<_arbitrarilyLargeMaxMagIndex; index++) {
                var magIdParam = _calMagIdParamFormat.replace("#", index)
                if (!controller.parameterExists(-1, magIdParam)) {
                    return index
                }
            }
            console.warn("SensorSetup.qml:currentMagParamCount internal error")
            return -1
        }
    }

    function currentExternalMagCount() {
        if (_allMagsDisabled) {
            return 0
        } else {
            var externalMagCount = 0
            for (var index=0; index<_arbitrarilyLargeMaxMagIndex; index++) {
                var magIdParam = _calMagIdParamFormat.replace("#", index)
                if (controller.parameterExists(-1, magIdParam)) {
                    var calMagIdFact = controller.getParameterFact(-1, magIdParam)
                    var calMagRotFact = controller.getParameterFact(-1, _calMagRotParamFormat.replace("#", index))
                    if (calMagIdFact.value > 0 && calMagRotFact.value >= 0) {
                        externalMagCount++
                    }
                } else {
                    return externalMagCount
                }
            }
            console.warn("SensorSetup.qml:currentExternalMagCount internal error")
            return 0
        }
    }

    function orientationsButtonVisible() {
        if (_sensorsHaveFixedOrientation || !showSetOrientations) {
            return false
        } else if (_boardOrientationChangeAllowed) {
            return true
        } else if (_compassOrientationChangeAllowed && !_allMagsDisabled) {
            for (var index=0; index<_arbitrarilyLargeMaxMagIndex; index++) {
                var magIdParam = _calMagIdParamFormat.replace("#", index)
                if (controller.parameterExists(-1, magIdParam)) {
                    var calMagIdFact = controller.parameterExists(-1, magIdParam)
                    var calMagRotFact = controller.parameterExists(-1, _calMagRotParamFormat.replace("#", index))
                    if (calMagIdFact.value > 0 && calMagRotFact.value >= 0) {
                        // Only external compasses can set orientation
                        return true
                    }
                }
            }
            return false
        } else {
            return false
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    SensorsComponentController {
        id:                         controller
        statusLog:                  statusTextArea
        progressBar:                progressBar
        orientationCalAreaHelpText: orientationCalAreaHelpText

        onResetStatusTextArea: statusLog.text = statusTextAreaDefaultText

        onMagCalComplete: {
            setOrientationsDialogShowBoardOrientation   = false
            setOrientationsDialogFactory.open({ title: qsTr("Compass Calibration Complete"), showRebootVehicleButton: true })
        }

        onWaitingForCancelChanged: {
            if (controller.waitingForCancel) {
                waitForCancelDialogFactory.open()
            }
        }

        onCalibrationActiveChanged: {
            if (controller.calibrationActive) {
                globals.navigationBlockedReason = qsTr("Complete or cancel the current calibration first")
            } else {
                globals.navigationBlockedReason = ""
            }
        }
    }

    Component.onDestruction: globals.navigationBlockedReason = ""

    QGCPopupDialogFactory {
        id: waitForCancelDialogFactory

        dialogComponent: waitForCancelDialogComponent
    }

    Component {
        id: waitForCancelDialogComponent

        QGCSimpleMessageDialog {
            title:      qsTr("Calibration Cancel")
            text:       qsTr("Waiting for Vehicle to response to Cancel. This may take a few seconds.")
            buttons:    0

            Connections {
                target: controller

                onWaitingForCancelChanged: {
                    if (!controller.waitingForCancel) {
                        close()
                    }
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: preCalibrationDialogFactory

        dialogComponent: preCalibrationDialogComponent
    }

    Component {
        id: preCalibrationDialogComponent

        QGCPopupDialog {
            buttons: Dialog.Cancel | Dialog.Ok

            onAccepted: {
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
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 50
                    Layout.preferredWidth:  innerColumn.width
                    wrapMode:               Text.WordWrap
                    text:                   preCalibrationDialogHelp
                }

                Column {
                    id:         innerColumn
                    spacing:    parent.spacing

                    QGCLabel {
                        id:         boardRotationHelp
                        wrapMode:   Text.WordWrap
                        visible:    !_sensorsHaveFixedOrientation && (preCalibrationDialogType == "accel" || preCalibrationDialogType == "compass")
                        text:       qsTr("Set autopilot orientation before calibrating.")
                    }

                    Column {
                        visible:    boardRotationHelp.visible
                        QGCLabel { text: qsTr("Autopilot Orientation") }

                        FactComboBox {
                            sizeToContents: true
                            fact:           sens_board_rot
                        }

                        QGCLabel {
                            wrapMode:   Text.WordWrap
                            text:       qsTr("ROTATION_NONE indicates component points in direction of flight.")
                        }
                    }

                    QGCLabel {
                        wrapMode:   Text.WordWrap
                        text:       qsTr("Click Ok to start calibration.")
                    }
                }
            }
        }
    }

    property bool setOrientationsDialogShowBoardOrientation:    true

    QGCPopupDialogFactory {
        id: setOrientationsDialogFactory

        dialogComponent: setOrientationsDialogComponent
    }

    Component {
        id: setOrientationsDialogComponent

        QGCPopupDialog {
            buttons: Dialog.Ok

            property bool showRebootVehicleButton: true

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    text:       qsTr("Reboot the vehicle prior to flight.")
                    visible:    showRebootVehicleButton
                }

                QGCButton {
                    text:       qsTr("Reboot Vehicle")
                    visible:    showRebootVehicleButton
                    onClicked: { controller.vehicle.rebootVehicle(); close() }
                }

                QGCLabel {
                    text:       qsTr("Adjust orientations as needed.\n\nROTATION_NONE indicates component points in direction of flight.")
                    visible:    _boardOrientationChangeAllowed || (_compassOrientationChangeAllowed && currentExternalMagCount() !== 0)
                }

                Column {
                    visible: _boardOrientationChangeAllowed

                    QGCLabel {
                        text: qsTr("Autopilot Orientation")
                    }

                    FactComboBox {
                        sizeToContents: true
                        fact:           sens_board_rot
                    }
                }

                Repeater {
                    model: _compassOrientationChangeAllowed ? currentMagParamCount() : 0

                    Column {
                        // id > = signals compass available, rot < 0 signals internal compass
                        visible: calMagIdFact.value > 0 && calMagRotFact.value >= 0

                        property Fact calMagIdFact:     controller.getParameterFact(-1, _calMagIdParamFormat.replace("#", index))
                        property Fact calMagRotFact:    controller.getParameterFact(-1, _calMagRotParamFormat.replace("#", index))

                        QGCLabel {
                            text: qsTr("Mag %1 Orientation").arg(index)
                        }

                        FactComboBox {
                            sizeToContents: true
                            fact:           parent.calMagRotFact
                        }
                    }
                }
            } // Column
        } // QGCPopupDialog
    } // Component - setOrientationsDialogComponent

    property string sectionNameFilter: ""

    function sectionVisible(name) {
        if (name === qsTr("Compass")) return !_allMagsDisabled && QGroundControl.corePlugin.options.showSensorCalibrationCompass && showSensorCalibrationCompass
        if (name === qsTr("Gyroscope")) return QGroundControl.corePlugin.options.showSensorCalibrationGyro && showSensorCalibrationGyro
        if (name === qsTr("Accelerometer")) return QGroundControl.corePlugin.options.showSensorCalibrationAccel && showSensorCalibrationAccel
        if (name === qsTr("Level Horizon")) return QGroundControl.corePlugin.options.showSensorCalibrationLevel && showSensorCalibrationLevel
        if (name === qsTr("Airspeed")) return vehicleComponent.airspeedCalSupported && QGroundControl.corePlugin.options.showSensorCalibrationAirspeed && showSensorCalibrationAirspeed
        if (name === qsTr("Orientations")) return orientationsButtonVisible()
        return true
    }

    function _startCalibration(type, help, title) {
        preCalibrationDialogType = type
        preCalibrationDialogHelp = help
        preCalibrationDialogFactory.open({ title: title })
    }

    property bool _showOrientationPreview: !controller.calibrationActive &&
        (sectionNameFilter === qsTr("Accelerometer") || sectionNameFilter === qsTr("Compass") || sectionNameFilter === qsTr("Gyroscope"))

    property bool _showAllSidesPreview: _showOrientationPreview &&
        (sectionNameFilter === qsTr("Accelerometer") || sectionNameFilter === qsTr("Compass"))

    property bool _showDownOnlyPreview: _showOrientationPreview &&
        sectionNameFilter === qsTr("Gyroscope")

    property bool _showStatusPreview: !controller.calibrationActive &&
        (sectionNameFilter === qsTr("Level Horizon") || sectionNameFilter === qsTr("Airspeed"))

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Calibration trigger buttons — one per section, shown based on sectionNameFilter
        ColumnLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 2
            visible: !controller.calibrationActive

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Calibrate Compass")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Compass")
                onClicked:  _startCalibration("compass", compassHelp, qsTr("Calibrate Compass"))
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Calibrate Gyroscope")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Gyroscope")
                onClicked:  _startCalibration("gyro", gyroHelp, qsTr("Calibrate Gyro"))
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Calibrate Accelerometer")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Accelerometer")
                onClicked:  _startCalibration("accel", accelHelp, qsTr("Calibrate Accelerometer"))
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Level Horizon")
                enabled:    cal_acc0_id.value !== 0 && cal_gyro0_id.value !== 0
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Level Horizon")
                onClicked:  _startCalibration("level", levelHelp, qsTr("Level Horizon"))
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Calibrate Airspeed")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Airspeed")
                onClicked:  _startCalibration("airspeed", airspeedHelp, qsTr("Calibrate Airspeed"))
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Set Orientations")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Orientations")
                onClicked: {
                    setOrientationsDialogShowBoardOrientation = true
                    setOrientationsDialogFactory.open({ title: qsTr("Set Orientations"), showRebootVehicleButton: false })
                }
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Factory Reset")
                visible:    sectionNameFilter === "" || sectionNameFilter === qsTr("Orientations")
                onClicked:  controller.resetFactoryParameters()
            }

            QGCButton {
                Layout.fillWidth: true
                text:       qsTr("Next")
                visible:    showNextButton
                onClicked:  _root.nextButtonClicked()
            }
        }

        // Active calibration area — visible during calibration
        RowLayout {
            Layout.fillWidth:   true
            visible:            controller.calibrationActive
            spacing:            ScreenTools.defaultFontPixelWidth

            ProgressBar {
                id:                 progressBar
                Layout.fillWidth:   true
            }

            QGCButton {
                text:       qsTr("Cancel")
                onClicked:  controller.cancelCalibration()
            }
        }

        Item {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            visible:            controller.calibrationActive || _showOrientationPreview || _showStatusPreview

            TextArea {
                id:             statusTextArea
                anchors.fill:   parent
                readOnly:       true
                visible:        !orientationCalArea.visible
                text:           statusTextAreaDefaultText
                color:          qgcPal.text
                background: Rectangle { color: qgcPal.windowShade }
            }

            Rectangle {
                id:         orientationCalArea
                anchors.fill: parent
                visible:    controller.showOrientationCalArea || _showOrientationPreview
                color:      qgcPal.windowShade

                QGCLabel {
                    id:                 orientationCalAreaHelpText
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.top:        orientationCalArea.top
                    anchors.left:       orientationCalArea.left
                    width:              parent.width
                    wrapMode:           Text.WordWrap
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    visible:            text !== ""
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
                        visible:            controller.orientationCalDownSideVisible || _showAllSidesPreview || _showDownOnlyPreview
                        calValid:           controller.orientationCalDownSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalDownSideInProgress
                        calInProgressText:  controller.orientationCalDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalDownSideRotate ? "qrc:///qmlimages/VehicleDownRotate.png" : "qrc:///qmlimages/VehicleDown.png"
                    }
                    VehicleRotationCal {
                        width:              parent.indicatorWidth
                        height:             parent.indicatorHeight
                        visible:            controller.orientationCalUpsideDownSideVisible || _showAllSidesPreview
                        calValid:           controller.orientationCalUpsideDownSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalUpsideDownSideInProgress
                        calInProgressText:  controller.orientationCalUpsideDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalUpsideDownSideRotate ? "qrc:///qmlimages/VehicleUpsideDownRotate.png" : "qrc:///qmlimages/VehicleUpsideDown.png"
                    }
                    VehicleRotationCal {
                        width:              parent.indicatorWidth
                        height:             parent.indicatorHeight
                        visible:            controller.orientationCalNoseDownSideVisible || _showAllSidesPreview
                        calValid:           controller.orientationCalNoseDownSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalNoseDownSideInProgress
                        calInProgressText:  controller.orientationCalNoseDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalNoseDownSideRotate ? "qrc:///qmlimages/VehicleNoseDownRotate.png" : "qrc:///qmlimages/VehicleNoseDown.png"
                    }
                    VehicleRotationCal {
                        width:              parent.indicatorWidth
                        height:             parent.indicatorHeight
                        visible:            controller.orientationCalTailDownSideVisible || _showAllSidesPreview
                        calValid:           controller.orientationCalTailDownSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalTailDownSideInProgress
                        calInProgressText:  controller.orientationCalTailDownSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalTailDownSideRotate ? "qrc:///qmlimages/VehicleTailDownRotate.png" : "qrc:///qmlimages/VehicleTailDown.png"
                    }
                    VehicleRotationCal {
                        width:              parent.indicatorWidth
                        height:             parent.indicatorHeight
                        visible:            controller.orientationCalLeftSideVisible || _showAllSidesPreview
                        calValid:           controller.orientationCalLeftSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalLeftSideInProgress
                        calInProgressText:  controller.orientationCalLeftSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalLeftSideRotate ? "qrc:///qmlimages/VehicleLeftRotate.png" : "qrc:///qmlimages/VehicleLeft.png"
                    }
                    VehicleRotationCal {
                        width:              parent.indicatorWidth
                        height:             parent.indicatorHeight
                        visible:            controller.orientationCalRightSideVisible || _showAllSidesPreview
                        calValid:           controller.orientationCalRightSideDone && !_showOrientationPreview
                        calInProgress:      controller.orientationCalRightSideInProgress
                        calInProgressText:  controller.orientationCalRightSideRotate ? qsTr("Rotate") : qsTr("Hold Still")
                        imageSource:        controller.orientationCalRightSideRotate ? "qrc:///qmlimages/VehicleRightRotate.png" : "qrc:///qmlimages/VehicleRight.png"
                    }
                }
            }
        }
    }
}
