import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4ConfigRows.js" as ConfigRows
import "PX4SensorsRows.js" as SensorsRows

/// Page for sensor calibration, sensor orientation and sensor options. This control is used within the SensorsComponent
/// control and can also be used standalone for custom UIs. When using standalone you can use the various show* bools to
/// show/hide what you want.
ColumnLayout {
    id:         _root
    spacing:    _margins

    property bool showSensorCalibrationCompass:     true    ///< true: Show the compass calibration
    property bool showSensorCalibrationGyro:        true    ///< true: Show the gyroscope calibration
    property bool showSensorCalibrationAccel:       true    ///< true: Show the accelerometer calibration
    property bool showSensorCalibrationLevel:       true    ///< true: Show the level horizon calibration
    property bool showSensorCalibrationAirspeed:    true    ///< true: Show the airspeed calibration

    /// Emitted when a calibration starts, so the host page can show the calibration panel at the top
    signal scrollToTopRequested

    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues: Object.create(null)

    // Calibrations which start from a dialog: the airspeed calibration
    property string _calibrationDialogType: ""
    property string _calibrationTitle:      ""
    property bool   _showCalibrationPanel:  false
    property bool   _dismissWhenFinished:   false
    property bool   _rebootNeeded:          false
    property string _largeVehicleResult:        ""
    property bool   _largeVehicleResultSuccess: false

    // The accelerometer and compass calibrations start from the panel: "accel" or "compass" while it waits for Start
    property string _calibrationKind:       ""
    property bool   _panelReady:            false
    property bool   _panelStarting:         false
    property string _panelStartMessage:     ""

    // A barometer or level horizon calibration runs in its row: "requested" until the vehicle starts it, then "done" or "failed"
    property string _inlineId:      ""
    property string _inlineState:   "idle"
    property string _inlineMessage: ""
    property bool   _inlineCancelled: false

    readonly property bool _inlineRunning: _inlineState === "requested" ||
                                           controller.activeCalibration === SensorsComponentController.ActiveCalibrationBaro ||
                                           controller.activeCalibration === SensorsComponentController.ActiveCalibrationLevel ||
                                           controller.activeCalibration === SensorsComponentController.ActiveCalibrationGyro
    // A calibration is running or has been asked for and the vehicle has not answered yet
    readonly property bool _calibrationBusy: controller.calibrationActive || _inlineRunning || _panelStarting ||
                                             controller.largeVehicleCalibrationPending

    // Leaving the page destroys the controller, which loses the result of a calibration that has been asked for
    on_CalibrationBusyChanged: globals.navigationBlockedReason = _calibrationBusy ? qsTr("Complete or cancel the current calibration first") : ""
    readonly property var  _gps: SensorsRows.gpsReadiness(
        _vehicle ? _vehicle.gps.lock.rawValue : 0,
        _vehicle ? _vehicle.gps.count.rawValue : 0,
        _vehicle ? _vehicle.gps.verticalAccuracy.rawValue : NaN)

    readonly property real  _margins:           ScreenTools.defaultFontPixelHeight * 0.5
    readonly property bool  _sensorsHaveFixedOrientation: QGroundControl.corePlugin.options.sensorsHaveFixedOrientation
    readonly property var   _vehicle:           controller.vehicle
    readonly property string _vehicleKind:      _vehicle ? (_vehicle.multiRotor ? "multiRotor" : _vehicle.vtol ? "vtol" : (_vehicle.fixedWing || _vehicle.airship) ? "fixedWing" : _vehicle.rover ? "rover" : "") : ""
    readonly property var   _vehicleKinds: ({
        "multiRotor":   _vehicle ? _vehicle.multiRotor : false,
        "fixedWing":    _vehicle ? (_vehicle.fixedWing || _vehicle.airship) : false,
        "vtol":         _vehicle ? _vehicle.vtol : false
    })
    readonly property var   _calibrationRows:   SensorsRows.calibrationRows(_vehicleKinds, _paramExists)
    readonly property var   _shownCalibrationRows: _calibrationRows.filter(
        row => SensorsRows.calibrationRowShown(row, _rawValue) && _calibrationAllowed(row))
    readonly property var   _rowsNeedingCalibration: SensorsRows.rowsNeedingCalibration(_shownCalibrationRows, _rawValue)
    readonly property var   _compassRows:       SensorsRows.compassRows(_rawValue, _paramExists)
    readonly property var   _devices:           SensorsRows.hardwareDevices(_rawValue, _paramExists)
    readonly property var   _rows:              ConfigRows.visibleRows(SensorsRows.sensorRows(), _vehicleKinds, _paramExists)
    readonly property var   _groups:            ConfigRows.groupRows(_rows)

    readonly property var _calibrations: ({
        "airspeed": {
            "title":        qsTr("Calibrate Airspeed"),
            "help":         qsTr("For Airspeed calibration you will need to keep your airspeed sensor out of any wind and then blow across the sensor. Do not touch the sensor or obstruct any holes during the calibration.")
        }
    })

    function _paramExists(name) {
        return controller.parameterExists(-1, name)
    }

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    function _calibrationAllowed(row) {
        const options = QGroundControl.corePlugin.options
        switch (row.calibration) {
        case "compass":
            return options.showSensorCalibrationCompass && showSensorCalibrationCompass
        case "gyro":
            return options.showSensorCalibrationGyro && showSensorCalibrationGyro
        case "accel":
            return options.showSensorCalibrationAccel && showSensorCalibrationAccel
        case "level":
            return options.showSensorCalibrationLevel && showSensorCalibrationLevel
        case "airspeed":
            // The component knows which parameters say there is an airspeed sensor on each firmware version
            return vehicleComponent && vehicleComponent.airspeedCalSupported &&
                   options.showSensorCalibrationAirspeed && showSensorCalibrationAirspeed
        default:
            return true
        }
    }

    function _groupShown(group) {
        return group.rows.some(row => SensorsRows.rowShown(row, _sensorsHaveFixedOrientation))
    }

    function _startCalibration(type) {
        _largeVehicleResult = ""
        if (type === "accel" || type === "compass" || type === "compassLargeVehicle") {
            _showPanelReady(type === "accel" ? "accel" : "compass", type === "compassLargeVehicle")
            return
        }
        _calibrationDialogType = type
        preCalibrationDialogFactory.open({ title: _calibrations[type].title })
    }

    // The panel shows what to do first, and the calibration starts from its Start button
    function _showPanelReady(kind, largeVehicleOpen) {
        _calibrationKind = kind
        _calibrationTitle = kind === "airspeed" ? qsTr("Airspeed") : ""
        _panelReady = true
        _panelStarting = false
        _panelStartMessage = ""
        calibrationPanel.largeVehicleOpen = largeVehicleOpen
        _showCalibrationPanel = true
        scrollToTopRequested()
    }

    function _startPanelCalibration() {
        _largeVehicleResult = ""
        _panelStarting = true
        _panelStartMessage = ""
        panelStartTimer.restart()
        switch (_calibrationKind) {
        case "accel":
            controller.calibrateAccel()
            break
        case "airspeed":
            controller.calibrateAirspeed()
            break
        default:
            controller.calibrateCompass()
            break
        }
    }

    // The vehicle starts the calibration within moments. It does not answer a request it refuses, such as when armed.
    Timer {
        id:         panelStartTimer
        interval:   4000

        onTriggered: {
            if (_root._panelStarting) {
                _root._panelStarting = false
                _root._panelStartMessage = qsTr("The vehicle did not start the calibration. It may be busy or armed.")
            }
        }
    }

    function _confirmReset() {
        if (_calibrationBusy) {
            return
        }
        QGroundControl.showMessageDialog(_root, qsTr("Reset sensor calibration"),
                                         qsTr("This clears the gyroscope, accelerometer and compass calibration and reboots the autopilot. Are you sure?"),
                                         Dialog.Yes | Dialog.Cancel,
                                         function () { controller.resetFactoryParameters() })
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    SensorsComponentController {
        id:                         controller

        onMagCalComplete: _root._rebootNeeded = true

        onLargeVehicleCalibrationFinished: (success, message) => {
            _root._largeVehicleResultSuccess = success
            _root._largeVehicleResult = success ? qsTr("Offsets set. They are saved when the vehicle disarms or reboots.") : message
        }

        onInlineCalibrationFinished: (success, message) => {
            const cancelled = _root._inlineCancelled
            _root._inlineCancelled = false
            if (cancelled && !success) {
                _root._inlineState = "idle"
                _root._inlineMessage = ""
                return
            }
            _root._inlineState = success ? "done" : "failed"
            _root._inlineMessage = success ? "" : SensorsRows.inlineFailureText(message)
        }

        onWaitingForCancelChanged: {
            if (controller.waitingForCancel) {
                waitForCancelDialogFactory.open()
            }
        }
    }

    Connections {
        target: controller

        function onCalibrationActiveChanged() {
            if (controller.calibrationActive) {
                if (_root._inlineRunning) {
                    inlineRequestTimer.stop()
                    return
                }
                panelStartTimer.stop()
                _root._panelReady = false
                _root._panelStarting = false
                _root._calibrationKind = _calibrationKey(controller.activeCalibration)
                _root._calibrationTitle = _root._calibrationKind === "airspeed" ? qsTr("Airspeed") : ""
                calibrationPanel.logOpen = _root._calibrationKind === "airspeed"
                _root._showCalibrationPanel = true
                _root.scrollToTopRequested()
            } else if (_root._dismissWhenFinished) {
                _root._dismissWhenFinished = false
                _root._showCalibrationPanel = false
            }
        }
    }

    function _startInlineCalibration(row) {
        // A panel left open from another calibration would stay up and disable this page while this one runs
        panelStartTimer.stop()
        _panelReady = false
        _panelStarting = false
        _showCalibrationPanel = false
        _inlineId = row.id
        _inlineState = "requested"
        _inlineMessage = ""
        _inlineCancelled = false
        inlineRequestTimer.restart()
        switch (row.calibration) {
        case "level":
            controller.calibrateLevel()
            break
        case "gyro":
            controller.calibrateGyro()
            break
        default:
            controller.calibrateBaro()
            break
        }
    }

    // The vehicle starts the calibration within moments. It does not answer a request it refuses, such as when armed.
    Timer {
        id:         inlineRequestTimer
        interval:   4000

        onTriggered: {
            if (_root._inlineState === "requested") {
                _root._inlineState = "failed"
                _root._inlineMessage = qsTr("The vehicle did not start the calibration. It may be busy or armed.")
            }
        }
    }

    function _calibrationKey(active) {
        switch (active) {
        case SensorsComponentController.ActiveCalibrationCompass:
            return "compass"
        case SensorsComponentController.ActiveCalibrationAccel:
            return "accel"
        case SensorsComponentController.ActiveCalibrationAirspeed:
            return "airspeed"
        default:
            return ""
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

                function onWaitingForCancelChanged() {
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
            id:                 dialog
            buttons:            Dialog.Cancel | Dialog.Ok

            onAccepted: {
                // The panel is shown first, so a request the vehicle does not answer is explained there and can be
                // retried. It also holds the pending state, so no other start can be made until the vehicle answers.
                _root._showPanelReady("airspeed", false)
                _root._startPanelCalibration()
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 50
                    wrapMode:               Text.WordWrap
                    text:                   _root._calibrations[_root._calibrationDialogType].help
                }

                QGCLabel {
                    wrapMode:   Text.WordWrap
                    text:       qsTr("Click Ok to start calibration.")
                }
            }
        }
    }

    Rectangle {
        objectName:         "sensors_statusBanner"
        Layout.fillWidth:   true
        implicitHeight:     bannerLayout.implicitHeight + _margins * 2
        color:              qgcPal.windowPanel
        border.color:       _rowsNeedingCalibration.length > 0 ? qgcPal.warningText : qgcPal.borderSubtle
        radius:             ScreenTools.defaultBorderRadius * 2
        visible:            !_showCalibrationPanel

        RowLayout {
            id:                 bannerLayout
            anchors.fill:       parent
            anchors.margins:    _margins
            spacing:            ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text:       _rowsNeedingCalibration.length > 0 ?
                                qsTr("%n sensor(s) need calibration before flight", "", _rowsNeedingCalibration.length) :
                                qsTr("Sensors are ready")
                color:      _rowsNeedingCalibration.length > 0 ? qgcPal.warningText : qgcPal.textPrimary
                font.bold:  true
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               _rowsNeedingCalibration.map(row => row.title).join(", ")
                color:              qgcPal.textSecondary
                elide:              Text.ElideRight
            }

            QGCLabel {
                text:       _largeVehicleResult
                color:      _largeVehicleResultSuccess ? qgcPal.successText : qgcPal.errorText
                visible:    _largeVehicleResult !== ""
            }

            QGCLabel {
                text:       qsTr("Reboot the vehicle before flight")
                color:      qgcPal.warningText
                visible:    _rebootNeeded
            }
        }
    }

    SensorsCalibrationPanel {
        id:                     calibrationPanel
        objectName:             "sensors_calibrationPanel"
        Layout.fillWidth:       true
        sensorsController:      controller
        kind:                   _calibrationKind
        title:                  _calibrationTitle
        ready:                  _panelReady
        starting:               _panelStarting
        startMessage:           _panelStartMessage
        gps:                    _gps
        vehicleKind:            _vehicleKind
        orientationFact:        _sensorsHaveFixedOrientation ? null : _fact("SENS_BOARD_ROT")
        largeVehicleMessage:    _largeVehicleResult
        largeVehicleSuccess:    _largeVehicleResultSuccess
        largeVehiclePending:    controller.largeVehicleCalibrationPending
        visible:                _showCalibrationPanel

        onStartRequested:       _root._startPanelCalibration()

        onCancelRequested: {
            _root._dismissWhenFinished = true
            controller.cancelCalibration()
        }

        onCloseRequested: {
            _root._panelReady = false
            _root._showCalibrationPanel = false
        }

        onDismissed:            _root._showCalibrationPanel = false

        onRebootRequested: {
            if (_root._vehicle) {
                _root._vehicle.rebootVehicle()
            }
        }

        onLargeVehicleRequested: (heading) => {
            _root._largeVehicleResult = ""
            controller.calibrateCompassLargeVehicle(heading)
        }
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            _margins

        QGCGroupBox {
            Layout.fillWidth:   true
            Layout.alignment:   Qt.AlignTop
            enabled:            !(controller.calibrationActive && _showCalibrationPanel)
            contentSpacing:     ScreenTools.defaultFontPixelHeight * 0.5
            showDividers:       false

            QGCGroupBoxTab {
                title:              qsTr("Calibration")
                buttonObjectName:   "sensors_tab_calibration"

                Repeater {
                    model: _shownCalibrationRows

                    SensorsCalibrationRow {
                        id:                 calibrationRow
                        showSeparator:      index > 0
                        row:                modelData
                        calibrationState:   SensorsRows.calibrationState(row, _root._rawValue)
                        inlineState:        row.id === _root._inlineId ? (_root._inlineRunning ? "running" : _root._inlineState) : "idle"
                        inlineMessage:      _root._inlineMessage
                        progress:           controller.calibrationProgress
                        gps:                _root._gps
                        calibrateEnabled:   !_root._calibrationBusy &&
                                            SensorsRows.calibrationEnabled(row, _root._calibrationRows, _root._rawValue)
                        cancelEnabled:      controller.calibrationActive && !controller.waitingForCancel
                        paramsEnabled:      !_root._calibrationBusy
                        factFor:            _root._fact
                        initialValues:      _root._initialValues

                        required property var modelData
                        required property int index

                        onCalibrateClicked: {
                            if (row.inline) {
                                _root._startInlineCalibration(row)
                            } else {
                                _root._startCalibration(row.calibration)
                            }
                        }

                        onCancelClicked: {
                            _root._inlineCancelled = true
                            controller.cancelCalibration()
                        }

                        onAlternateClicked: _root._startCalibration(row.alternate.type)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    visible:            _compassRows.length > 0

                    ConfigGroupHeader {
                        text: qsTr("Compasses")
                    }

                    Repeater {
                        model: _compassRows

                        ConfigRow {
                            id:             compassRow
                            objectName:     "sensors_row_" + row.id
                            title:          row.title
                            params:         row.params
                            showSeparator:  index > 0
                            enabled:        !_root._calibrationBusy

                            required property var modelData
                            required property int index

                            readonly property var   row:        modelData
                            readonly property bool  _external:  SensorsRows.compassExternal(_root._rawValue(row.rotationParam))

                            // PX4 puts the rotation back to custom while a custom angle is not zero
                            Connections {
                                target: _root._fact(compassRow.row.rotationParam)

                                function onRawValueChanged() {
                                    // A calibration which sets the rotation is the firmware's own change
                                    if (_root._calibrationBusy) {
                                        return
                                    }
                                    SensorsRows.staleCustomAngles(compassRow.row, _root._rawValue).forEach(
                                        param => _root._fact(param).rawValue = 0)
                                }
                            }

                            cellsData: [
                                QGCCaptionCell {
                                    caption: qsTr("Device")

                                    RowLayout {
                                        spacing: ScreenTools.defaultFontPixelWidth

                                        QGCLabel {
                                            text:           controller.deviceDescription(_root._rawValue(compassRow.row.idParam))
                                            font.family:    ScreenTools.fixedFontFamily
                                        }

                                        QGCChip {
                                            text: compassRow._external ? qsTr("External") : qsTr("Internal")
                                        }
                                    }
                                },

                                Repeater {
                                    model: compassRow.row.cells

                                    ConfigParamCell {
                                        cell:               modelData
                                        factFor:            _root._fact
                                        initialValues:      _root._initialValues
                                        objectNamePrefix:   "sensors"
                                        visible:            (!modelData.externalOnly || compassRow._external) && SensorsRows.cellShown(modelData, _root._rawValue)
                                        enabled:            !modelData.externalOnly || !_root._sensorsHaveFixedOrientation

                                        required property var modelData
                                    }
                                }
                            ]
                        }
                    }
                }

                ConfigRowGroups {
                    model:              SensorsRows.tabGroups(_groups, false)
                    groupShown:         _root._groupShown
                    objectNamePrefix:   "sensors"
                    rowDelegate:        paramRowComponent
                }
            }

            QGCGroupBoxTab {
                title:              qsTranslate("SetupPage", "Advanced")
                buttonObjectName:   "sensors_tab_advanced"

                ConfigRowGroups {
                    model:              SensorsRows.tabGroups(_groups, true)
                    groupShown:         _root._groupShown
                    objectNamePrefix:   "sensors"
                    rowDelegate:        paramRowComponent
                }

                ConfigRow {
                    objectName:     "sensors_row_reset"
                    title:          qsTr("Reset calibration")
                    showSeparator:  true

                    ConfigNote {
                        text: qsTr("Clears the gyroscope, accelerometer and compass calibration and reboots the autopilot.")
                    }

                    cellsData: QGCCaptionCell {
                        QGCButton {
                            objectName: "sensorsSetup_factoryReset"
                            text:       qsTr("Factory Reset")
                            enabled:    !_root._calibrationBusy
                            onClicked:  _root._confirmReset()
                        }
                    }
                }
            }
        }

        Component {
            id: paramRowComponent

            ConfigRow {
                id:             paramRow
                objectName:     "sensors_row_" + row.id
                title:          row.title
                params:         row.params
                showSeparator:  index > 0
                visible:        SensorsRows.rowShown(row, _root._sensorsHaveFixedOrientation)
                enabled:        !_root._calibrationBusy

                required property var modelData
                required property int index

                readonly property var row: modelData

                ConfigNote {
                    text: paramRow.row.description !== undefined ? paramRow.row.description : ""
                }

                cellsData: Repeater {
                    model: paramRow.row.cells

                    ConfigParamCell {
                        cell:               modelData
                        factFor:            _root._fact
                        initialValues:      _root._initialValues
                        objectNamePrefix:   "sensors"
                        visible:            SensorsRows.cellShown(modelData, _root._rawValue)

                        required property var modelData
                    }
                }
            }
        }

        SensorsLiveCard {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 36
            Layout.alignment:       Qt.AlignTop
            visible:                _root.width > ScreenTools.defaultFontPixelWidth * 110
            vehicle:                _root._vehicle
            vehicleKind:            _root._vehicleKind
            devices:                _root._devices
            deviceName:             function (param) {
                const value = _root._rawValue(param)
                return value === undefined ? "" : controller.deviceDescription(value)
            }
        }
    }
}
