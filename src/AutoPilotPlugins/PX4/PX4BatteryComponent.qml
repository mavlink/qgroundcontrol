import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

import "PX4BatteryRows.js" as BatteryRows
import "PX4ConfigRows.js" as ConfigRows

SetupPage {
    id:             batteryPage
    pageComponent:  pageComponent

    property bool _showInUseOnly:   false
    property bool _filterChosen:    false
    // Parameter values when the page first showed them, so a reboot is only flagged after a change
    property var _initialValues:    Object.create(null)

    readonly property var _vehicle: controller.vehicle
    readonly property bool _estimatorOverwrite: BatteryRows.overwriteInEstimator(_vehicle ? _vehicle.firmwareMajorVersion : -1,
                                                                                 _vehicle ? _vehicle.firmwareMinorVersion : -1)
    readonly property bool _loading: controller.pendingParams.length > 0
    readonly property var _allRows: BatteryRows.batteryRows()
    readonly property var _rows: {
        // Rebuilt only when the vehicle adds a parameter. Each rebuild recreates the row controls, and a read the
        // firmware cannot answer changes nothing visible.
        controller.addedParamCount
        return BatteryRows.visibleRows(_allRows, name => controller.parameterReady(name))
    }
    readonly property var _groups: ConfigRows.groupRows(_rows)
    readonly property var _reportedTelemetry: {
        const reported = []
        const batteries = _vehicle ? _vehicle.batteries : null
        for (let i = 0; batteries && i < batteries.count; i++) {
            reported.push(batteries.get(i))
        }
        return reported
    }
    readonly property var _otherTelemetry: {
        const batteries = _rows.filter(row => row.instance !== undefined)
                               .map(row => ({ "instance": row.instance, "inputs": _rowInputs(row) }))
        const unclaimed = BatteryRows.unclaimedTelemetryIds(_reportedTelemetry.map(battery => battery.id.rawValue), batteries)
        return _reportedTelemetry.filter(battery => unclaimed.indexOf(battery.id.rawValue) >= 0)
    }
    readonly property var _filterableRows: _rows.filter(row => !row.shared)
    readonly property int _inUseCount: _filterableRows.filter(row => _rowActive(row)).length

    function _fact(name) {
        return controller.getParameterFact(-1, name, false)
    }

    function _rawValue(name) {
        const fact = _fact(name)
        return fact ? fact.rawValue : undefined
    }

    function _rowActive(row) {
        return BatteryRows.isRowActive(row, _rawValue)
    }

    function _rowShown(row) {
        return !_showInUseOnly || row.shared === true || _rowActive(row)
    }

    function _groupShown(group) {
        return group.rows.some(row => _rowShown(row))
    }

    function _rowInputs(row) {
        return row.sourceParam ? BatteryRows.batteryInputs(_rawValue(row.sourceParam), _rawValue("UAVCAN_SUB_BAT"),
                                                           _rawValue("UAVCAN_ENABLE"), _estimatorOverwrite) : []
    }

    function _sourceDescription(inputs) {
        if (inputs.indexOf("canFiltered") >= 0) {
            return qsTr("DroneCAN data filtered through the battery estimate below.")
        }
        if (inputs.indexOf("external") >= 0) {
            return qsTr("Reported by MAVLink, DroneCAN or a driver, which estimates the charge itself.")
        }
        if (inputs.indexOf("escs") >= 0) {
            return qsTr("Voltage and current from ESC telemetry.")
        }
        if (inputs.indexOf("powerModule") >= 0) {
            return qsTr("Analog power module or I2C power monitor. The analog settings only apply to the board's ADC inputs.")
        }
        return qsTr("Not monitored.")
    }

    function _batteryTelemetry(telemetryId) {
        if (telemetryId < 0) {
            return null
        }
        for (const battery of _reportedTelemetry) {
            if (battery.id.rawValue === telemetryId) {
                return battery
            }
        }
        return null
    }

    function _telemetryText(battery) {
        return [battery.voltage, battery.percentRemaining, battery.current]
            .map(fact => fact.valueString + " " + fact.units)
            .join("   ")
    }

    // Reads pendingParams from the controller, not _loading, which may not have updated yet when pendingParams changes
    function _chooseFilter() {
        if (!_filterChosen && controller.pendingParams.length === 0) {
            _filterChosen = true
            _showInUseOnly = _inUseCount > 0
        }
    }

    PX4BatteryController {
        id:                     controller
        // Reboot-required settings are marked on the page instead
        suppressRebootMessage:  true

        // Opening on what is in use hides the spare battery slots and unused power monitor drivers
        onPendingParamsChanged: _chooseFilter()
    }

    QGCPalette {
        id: qgcPal
    }

    QGCPopupDialogFactory {
        id:                 voltageDividerDialogFactory
        dialogComponent:    voltageDividerDialogComponent
    }

    Component {
        id: voltageDividerDialogComponent

        CalcVoltageDividerDialog { }
    }

    QGCPopupDialogFactory {
        id:                 ampsPerVoltDialogFactory
        dialogComponent:    ampsPerVoltDialogComponent
    }

    Component {
        id: ampsPerVoltDialogComponent

        CalcAmpsPerVoltDialog { }
    }

    Component {
        id: pageComponent

        ColumnLayout {
            width:   Math.max(availableWidth, implicitWidth)
            spacing: _margins

            ConfigRowsPanel {
                ConfigRowsFilterBar {
                    objectName:         "battery_filter"
                    filterLabels:       [ qsTr("All"), qsTr("In Use") ]
                    filterIndex:        _showInUseOnly ? 1 : 0
                    summary: {
                        const total = _filterableRows.length
                        if (_showInUseOnly) {
                            return total === 1 ?
                                       qsTr("Showing %1 of 1 battery or power monitor.").arg(_inUseCount) :
                                       qsTr("Showing %1 of %2 batteries and power monitors.").arg(_inUseCount).arg(total)
                        }
                        return total === 1 ?
                                   qsTr("Showing the only battery or power monitor.") :
                                   qsTr("Showing all %1 batteries and power monitors.").arg(total)
                    }
                    onFilterActivated: (index) => {
                        _filterChosen = true
                        _showInUseOnly = index === 1
                    }

                    QGCLabel {
                        objectName: "battery_loading"
                        color:      qgcPal.textSecondary
                        text:       qsTr("Loading settings from the vehicle...")
                        visible:    _loading
                    }
                }

                ConfigRowGroups {
                    model:      _groups
                    groupShown: batteryPage._groupShown

                    rowDelegate: ConfigRow {
                        id:             rowLayout
                        objectName:     "battery_row_" + row.id
                        title:          row.title
                        params:         BatteryRows.usedParams(row, _inputs)
                        active:         _rowActive(row)
                        showSeparator:  index > 0
                        visible:        _rowShown(row)

                        required property var modelData
                        required property int index

                        readonly property var row:          modelData
                        readonly property bool _isBattery:  row.instance !== undefined
                        readonly property var _inputs:      _rowInputs(row)
                        readonly property var _telemetry:   _isBattery ? _batteryTelemetry(BatteryRows.telemetryId(row.instance, _inputs)) : null
                        readonly property string _warning:  _isBattery ? BatteryRows.sourceWarning(row.instance, _rawValue(row.sourceParam)) : ""

                        QGCLabel {
                            objectName:     "battery_telemetry_" + rowLayout.row.id
                            text:           rowLayout._telemetry ? _telemetryText(rowLayout._telemetry) : ""
                            font.family:    ScreenTools.fixedFontFamily
                            visible:        rowLayout._telemetry !== null
                        }

                        ConfigNote {
                            text: rowLayout._isBattery ? _sourceDescription(rowLayout._inputs) : rowLayout.row.description
                        }

                        ConfigNote {
                            color:  qgcPal.warningText
                            text:   rowLayout._warning
                        }

                        cellsData: Repeater {
                            model: rowLayout.row.cells

                            ConfigParamCell {
                                id:                 paramCell
                                cell:               modelData
                                factFor:            batteryPage._fact
                                initialValues:      batteryPage._initialValues
                                objectNamePrefix:   "battery"
                                numberTrailing:     modelData.calc !== undefined ? calcButtonComponent : null
                                visible:            BatteryRows.cellUsed(modelData, rowLayout._inputs)

                                required property var modelData

                                Component {
                                    id: calcButtonComponent

                                    QGCButton {
                                        objectName: "battery_calc_" + paramCell.modelData.param
                                        text:       qsTr("Calculate")
                                        // Calculating scales the current value, so it needs a set value rather than the board default
                                        enabled:    rowLayout._telemetry !== null && paramCell.fact !== null && paramCell.fact.rawValue > 0
                                        onClicked: {
                                            const factory = paramCell.modelData.calc === "voltageDivider" ?
                                                                voltageDividerDialogFactory : ampsPerVoltDialogFactory
                                            factory.open({ "batteryIndex": rowLayout.row.instance })
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Reported under an id that does not identify a battery row above
                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            _margins
                    visible:            _otherTelemetry.length > 0

                    ConfigGroupHeader {
                        text: qsTr("Other Reported Batteries")
                    }

                    ConfigNote {
                        text: qsTr("Batteries the vehicle reports that are not linked to a battery above, such as External sources. DroneCAN batteries report their node ID minus one, and MAVLink batteries 255.")
                    }

                    Repeater {
                        model: _otherTelemetry

                        RowLayout {
                            objectName: "battery_other_" + modelData.id.rawValue
                            spacing:    ScreenTools.defaultFontPixelWidth * 2

                            required property var modelData

                            QGCLabel {
                                text:       qsTr("ID %1").arg(modelData.id.rawValue)
                                font.bold:  true
                            }

                            QGCLabel {
                                text:           _telemetryText(modelData)
                                font.family:    ScreenTools.fixedFontFamily
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        controller.requestParameters(BatteryRows.allParams(_allRows))
        _chooseFilter()
    }
}
