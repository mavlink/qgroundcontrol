import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4SensorsOrientation.js" as Orientation
import "PX4SensorsRows.js" as SensorsRows

/// The calibration which has the page's attention: the ready screen before it starts, then its progress, and its result.
/// An accelerometer or compass calibration shows the sides of the vehicle, the others show the log. sensorsController is the
/// page's SensorsComponentController, and the page handles the signals.
ConfigRowsPanel {
    id: control

    required property var sensorsController

    /// "accel", "compass" or "airspeed"
    property string kind:       ""
    property string title:      ""
    /// True until the calibration is started, so the panel shows what to do first
    property bool   ready:      false
    /// Start was clicked and the vehicle has not answered yet
    property bool   starting:   false
    /// Why the vehicle did not start the calibration, empty if it did
    property string startMessage:   ""
    /// SensorsRows.gpsReadiness(), for the large vehicle calibration
    property var    gps:        SensorsRows.gpsReadiness(0, 0, NaN)
    /// The autopilot orientation fact, which has to be set before a calibration. Null when it is fixed.
    property var    orientationFact:        null
    property string vehicleKind:            ""
    property alias  largeVehicleOpen:       largeVehicleForm.open
    property string largeVehicleMessage:    ""
    property bool   largeVehicleSuccess:    false
    property bool   largeVehiclePending:    false
    property bool   logOpen:                false

    /// Start, or retry after a failure
    signal startRequested
    signal cancelRequested
    /// Cancel before the calibration started, or Close after a failure
    signal closeRequested
    /// Done after a success
    signal dismissed
    signal rebootRequested
    signal largeVehicleRequested(real heading)

    readonly property bool      _orientationKind:   kind === "accel" || kind === "compass"
    readonly property string    _kindName:          kind === "accel" ? qsTr("Accelerometer") : kind === "compass" ? qsTr("Compass") : title
    readonly property string    _phase: {
        if (ready) {
            return "ready"
        }
        if (sensorsController.calibrationActive) {
            return sensorsController.waitingForCancel ? "cancelling" : "running"
        }
        if (sensorsController.calibrationResult === SensorsComponentController.CalibrationResultSuccess) {
            return "done"
        }
        if (sensorsController.calibrationResult === SensorsComponentController.CalibrationResultFailed) {
            return "failed"
        }
        return "ready"
    }

    // The sides which the calibration uses and where each is, read again when the controller reports a change
    property int    _revision:  0
    property string _current:   ""      ///< The side the vehicle was last on
    property string _lastDone:  ""      ///< The side which was finished last, until another side is detected
    readonly property var _states: _readStates(_revision)
    readonly property var _sideIds: Orientation.sideIds.filter(side => _states[side].used)
    readonly property var _doneSides: _sideIds.filter(side => _states[side].state === 3)
    readonly property string _detected: {
        const active = _sideIds.filter(side => _states[side].state === 2)
        return active.length > 0 ? active[0] : ""
    }
    readonly property string _suggested: {
        const unused = Orientation.sideIds.filter(side => _sideIds.indexOf(side) < 0)
        return Orientation.suggestNext(_current, _doneSides.concat(unused))
    }
    readonly property var _callout: _makeCallout()

    function _readStates(revision) {
        const states = {}
        for (const side of Orientation.sideIds) {
            states[side] = {
                used:   _phase === "ready" || sensorsController[Orientation.visibleProperty(side)],
                state:  _phase === "ready" ? 0 : sensorsController[Orientation.stateProperty(side)]
            }
        }
        return states
    }

    function _sideState(side) {
        return _states[side].state
    }

    function _failureText() {
        const text = sensorsController.calibrationFailureText
        const known = SensorsRows.inlineFailureText(text)
        if (kind === "airspeed") {
            // The log below has the firmware's reason when it is not one which is recognized
            return known !== text ? known : qsTr("The calibration did not finish. Keep the sensor out of the wind, blow across it when asked, then try again.")
        }
        if (known !== text && text !== "") {
            return known
        }
        return kind === "compass" ? qsTr("The readings were not consistent. Move away from metal and magnets, then try again.") :
                                    qsTr("The readings could not be used. Keep the vehicle still on each side, then try again.")
    }

    // kind is "plain", "acc", "ok" or "err"
    function _makeCallout() {
        switch (_phase) {
        case "ready":
            if (starting) {
                return { kind: "plain", icon: "…", big: qsTr("Starting…"), sub: qsTr("Waiting for the vehicle.") }
            }
            if (startMessage !== "") {
                return { kind: "err", icon: "!", big: qsTr("Could not start"), sub: startMessage }
            }
            if (!_orientationKind) {
                return { kind: "plain", icon: "▶", big: qsTr("Ready to calibrate"), sub: "" }
            }
            return { kind: "plain", icon: "", big: qsTr("Ready to calibrate"),
                     sub: kind === "accel" ? qsTr("Put the vehicle on each of its sides, in any order, and hold it still on each.") :
                                             qsTr("Move away from metal and magnets. On each side, turn the vehicle slowly until the side is marked done.") }
        case "cancelling":
            return { kind: "plain", icon: "…", big: qsTr("Waiting for the vehicle to cancel"), sub: qsTr("This can take a few seconds.") }
        case "done":
            return { kind: "ok", icon: "✓", big: qsTr("%1 calibrated").arg(_kindName),
                     sub: kind === "compass" ? qsTr("Reboot the vehicle before flight so it takes effect.") : qsTr("The result is saved on the vehicle.") }
        case "failed":
            return { kind: "err", icon: "!", big: qsTr("Calibration failed"), sub: _failureText() }
        }

        if (!_orientationKind) {
            return { kind: "acc", icon: "…", big: qsTr("Calibrating %1").arg(_kindName.toLowerCase()), sub: qsTr("Follow the instructions in the log.") }
        }
        if (_detected !== "") {
            return kind === "compass" ? { kind: "acc", icon: "↻", big: qsTr("Rotate the vehicle slowly"), sub: qsTr("Keep turning until the side is marked done.") } :
                                        { kind: "acc", icon: "✋", big: qsTr("Hold still"), sub: qsTr("Do not move it until the side is marked done.") }
        }
        if (sensorsController.sideAlreadyDone) {
            return { kind: "acc", icon: "!", big: qsTr("That side is already done"), sub: qsTr("Use a side that is not done yet.") }
        }
        if (_lastDone !== "") {
            return { kind: "ok", icon: "✓", big: qsTr("%1 done").arg(Orientation.sideName(_lastDone)),
                     sub: _suggested !== "" ? qsTr("Now place it on another side.") : "" }
        }
        return { kind: "acc", icon: "→", big: qsTr("Next: %1").arg(Orientation.sideName(_suggested)), sub: "" }
    }

    function _sideObjectName(side) {
        return {
            level:   "sensorsCal_downSide",
            flipped: "sensorsCal_upsideDownSide",
            nose:    "sensorsCal_noseDownSide",
            tail:    "sensorsCal_tailDownSide",
            left:    "sensorsCal_leftSide",
            right:   "sensorsCal_rightSide"
        }[side]
    }

    function _refresh() {
        const wasDone = _doneSides
        _revision++
        if (_detected !== "") {
            _current = _detected
            _lastDone = ""
        }
        const newlyDone = _doneSides.filter(side => wasDone.indexOf(side) < 0)
        if (newlyDone.length > 0) {
            _lastDone = newlyDone[newlyDone.length - 1]
        }
    }

    QGCPalette { id: qgcPal }

    Connections {
        target: control.sensorsController

        function onOrientationCalSidesStateChanged() { control._refresh() }
        function onOrientationCalSidesVisibleChanged() { control._revision++ }

        // A new calibration, or the ready screen after one ends, starts again from level
        function onCalibrationActiveChanged() {
            control._current = ""
            control._lastDone = ""
            control._revision++
        }

        function onCalibrationResultChanged() {
            control._revision++
            if (control.sensorsController.calibrationResult === SensorsComponentController.CalibrationResultFailed) {
                control.logOpen = true
            }
        }
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth

        Rectangle {
            implicitWidth:  ScreenTools.defaultFontPixelHeight * 0.5
            implicitHeight: implicitWidth
            radius:         implicitWidth / 2
            color:          control._phase === "done" ? qgcPal.colorGreen :
                                control._phase === "failed" ? qgcPal.colorRed :
                                (control._phase === "running" || control._phase === "cancelling") ? qgcPal.accent : "transparent"
            border.color:   color === "transparent" ? qgcPal.textSecondary : color
        }

        QGCLabel {
            objectName:             "sensorsCal_title"
            text:                   control._kindName
            font.bold:              true
            font.capitalization:    Font.AllUppercase
            font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.1
        }

        QGCChip {
            objectName: "sensorsCal_counter"
            textColor:  control._phase === "done" ? qgcPal.successText : control._phase === "failed" ? qgcPal.errorText :
                            (control._phase === "running" || control._phase === "cancelling") ? qgcPal.accent : qgcPal.textSecondary
            visible:    control._orientationKind
            text: {
                switch (control._phase) {
                case "done":
                    return qsTr("Calibrated")
                case "failed":
                    return qsTr("Failed")
                case "ready":
                    return qsTr("%1 sides").arg(control._sideIds.length)
                }
                return qsTr("%1 of %2 sides done").arg(control._doneSides.length).arg(control._sideIds.length)
            }
        }

        Item { Layout.fillWidth: true }

        QGCButton {
            objectName: control._phase === "running" || control._phase === "cancelling" ? "sensorsSetup_cancelCalibration" : "sensorsSetup_closeCalibration"
            text:       control._phase === "cancelling" ? qsTr("Cancelling…") : control._phase === "failed" ? qsTr("Close") : qsTr("Cancel")
            enabled:    control._phase !== "cancelling" && !control.starting
            visible:    control._phase !== "done"
            onClicked:  control._phase === "running" ? control.cancelRequested() : control.closeRequested()
        }
    }

    // One segment for each side. The firmware does not report progress within a side.
    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth * 0.5
        visible:            control._orientationKind

        Repeater {
            model: control._sideIds

            Rectangle {
                id: segment

                required property string modelData
                readonly property int sideState: control._sideState(modelData)

                objectName:             "sensorsCal_segment_" + modelData
                Layout.fillWidth:       true
                implicitHeight:         ScreenTools.defaultFontPixelHeight * 0.5
                radius:                 height / 2
                color:                  sideState === 3 ? qgcPal.colorGreen : sideState === 2 ? qgcPal.accent : qgcPal.windowInset
                border.color:           sideState === 3 ? qgcPal.colorGreen : sideState === 2 ? qgcPal.accent : qgcPal.borderSubtle
            }
        }
    }

    ProgressBar {
        objectName:         "sensorsCal_progress"
        Layout.fillWidth:   true
        value:              control.sensorsController.calibrationProgress
        visible:            !control._orientationKind
    }

    Rectangle {
        id: callout

        objectName:         "sensorsCal_callout"
        Layout.fillWidth:   true
        implicitHeight:     calloutLayout.implicitHeight + ScreenTools.defaultFontPixelHeight
        radius:             ScreenTools.defaultBorderRadius
        color:              control._callout.kind === "ok" ? Qt.alpha(qgcPal.colorGreen, 0.12) :
                                control._callout.kind === "err" ? Qt.alpha(qgcPal.colorRed, 0.12) :
                                control._callout.kind === "acc" ? qgcPal.accentSubtle : qgcPal.windowCard
        border.color:       control._callout.kind === "ok" ? qgcPal.colorGreen :
                                control._callout.kind === "err" ? qgcPal.colorRed :
                                control._callout.kind === "acc" ? qgcPal.accent : qgcPal.borderSubtle

        RowLayout {
            id:                 calloutLayout
            anchors.fill:       parent
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            spacing:            ScreenTools.defaultFontPixelWidth * 1.5

            Rectangle {
                implicitWidth:  ScreenTools.defaultFontPixelHeight * 2
                implicitHeight: implicitWidth
                radius:         implicitWidth / 2
                color:          control._callout.kind === "ok" ? qgcPal.colorGreen : control._callout.kind === "err" ? qgcPal.colorRed :
                                    control._callout.kind === "acc" ? qgcPal.accent : qgcPal.windowInset
                border.color:   qgcPal.borderSubtle
                visible:        control._callout.icon !== ""

                QGCLabel {
                    anchors.centerIn:   parent
                    text:               control._callout.icon
                    font.bold:          true
                    color:              control._callout.kind === "plain" ? qgcPal.textSecondary : qgcPal.window
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            0

                QGCLabel {
                    objectName:         "sensorsCal_calloutTitle"
                    Layout.fillWidth:   true
                    text:               control._callout.big
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    font.bold:          true
                    wrapMode:           Text.WordWrap
                    color:              control._callout.kind === "ok" ? qgcPal.successText : control._callout.kind === "err" ? qgcPal.errorText : qgcPal.textPrimary
                }

                QGCLabel {
                    objectName:         "sensorsCal_calloutText"
                    Layout.fillWidth:   true
                    text:               control._callout.sub
                    color:              qgcPal.textSecondary
                    wrapMode:           Text.WordWrap
                    visible:            text !== ""
                }
            }

            QGCButton {
                objectName: "sensorsSetup_startCalibration"
                text:       qsTr("Start")
                primary:    true
                visible:    control._phase === "ready"
                enabled:    !control.starting && !control.largeVehiclePending
                onClicked:  control.startRequested()
            }

            QGCButton {
                objectName: "sensorsSetup_retryCalibration"
                text:       qsTr("Retry")
                primary:    true
                visible:    control._phase === "failed"
                onClicked:  control.startRequested()
            }

            QGCButton {
                objectName: "sensorsSetup_rebootVehicle"
                text:       qsTr("Reboot vehicle")
                primary:    true
                visible:    control._phase === "done" && control.kind === "compass"
                onClicked:  control.rebootRequested()
            }

            QGCButton {
                objectName: "sensorsSetup_dismissCalibration"
                text:       qsTr("Done")
                primary:    control.kind !== "compass"
                visible:    control._phase === "done"
                onClicked:  control.dismissed()
            }
        }
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            control._orientationKind

        SensorsCalibrationStage {
            vehicleKind:            control.vehicleKind
            Layout.fillWidth:       true
            Layout.fillHeight:      true
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 40
            side: {
                if (control._phase === "done" || control._phase === "failed") {
                    return "level"
                }
                return control._detected !== "" ? control._detected : (control._suggested !== "" ? control._suggested : "level")
            }
            rotating:   control.kind === "compass" && control._detected !== "" && control._phase === "running"
            instruction: {
                if (control._phase !== "running" || control._detected === "") {
                    return ""
                }
                return control.kind === "compass" ? qsTr("Rotate") : qsTr("Hold still")
            }
            active:     control._detected !== ""
            mark:       control._phase === "done" ? "ok" : control._phase === "failed" ? "error" : ""
            tag: {
                if (control._phase === "ready") {
                    return qsTr("Start here")
                }
                if (control._phase !== "running") {
                    return ""
                }
                return control._detected !== "" ? qsTr("Detected") : qsTr("Suggested")
            }
            caption: {
                if (control._phase === "done") {
                    return qsTr("All sides done")
                }
                if (control._phase === "failed") {
                    return qsTr("Calibration incomplete")
                }
                return Orientation.sideName(control._detected !== "" ? control._detected : control._suggested)
            }
        }

        ColumnLayout {
            Layout.fillWidth:       true
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 34
            Layout.alignment:       Qt.AlignTop
            spacing:                ScreenTools.defaultFontPixelHeight * 0.3

            Repeater {
                model: control._sideIds

                SensorsSideRow {
                    required property string modelData

                    objectName:         control._sideObjectName(modelData)
                    side:               modelData
                    calState:           control._sideState(modelData)
                    suggested:          control._phase !== "done" && control._phase !== "failed" && control._detected === "" && modelData === control._suggested
                }
            }
        }
    }

    RowLayout {
        objectName:         "sensorsCal_orientationRow"
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            control._phase === "ready" && control.orientationFact !== null
        enabled:            !control.starting && !control.largeVehiclePending

        QGCLabel {
            text:   qsTr("Autopilot orientation")
        }

        FactComboBox {
            sizeToContents: true
            fact:           control.orientationFact
        }

        QGCLabel {
            text:   qsTr("Set this before calibrating.")
            color:  qgcPal.textSecondary
        }
    }

    SensorsLargeVehicleForm {
        id:                 largeVehicleForm
        Layout.fillWidth:   true
        gps:                control.gps
        message:            control.largeVehicleMessage
        success:            control.largeVehicleSuccess
        pending:            control.largeVehiclePending
        visible:            control.kind === "compass" && control._phase === "ready"
        enabled:            !control.starting

        onCalibrateRequested:       (heading) => control.largeVehicleRequested(heading)
        onRebootRequested:          control.rebootRequested()
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            control._phase !== "ready" && (!control._orientationKind || control._phase === "failed")

        QGCLabel {
            objectName: "sensorsCal_logToggle"
            text:       (control.logOpen ? qsTr("Hide log") + " ▾" : qsTr("Show log") + " ▸")
            color:      qgcPal.accent

            TapHandler {
                onTapped: control.logOpen = !control.logOpen
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            text:               control.sensorsController.calibrationLog.split("\n").pop()
            font.family:        ScreenTools.fixedFontFamily
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.textSecondary
            elide:              Text.ElideRight
            visible:            !control.logOpen
        }
    }

    ScrollView {
        Layout.fillWidth:       true
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 9
        visible:                control._phase !== "ready" && (!control._orientationKind || control._phase === "failed") && control.logOpen

        TextArea {
            objectName:     "sensorsCal_log"
            readOnly:       true
            text:           control.sensorsController.calibrationLog
            font.family:    ScreenTools.fixedFontFamily
            font.pointSize: ScreenTools.smallFontPointSize
            color:          qgcPal.textPrimary
            wrapMode:       TextEdit.Wrap

            background: Rectangle {
                color:          qgcPal.windowInset
                border.color:   qgcPal.borderSubtle
                radius:         ScreenTools.defaultBorderRadius
            }
        }
    }
}
