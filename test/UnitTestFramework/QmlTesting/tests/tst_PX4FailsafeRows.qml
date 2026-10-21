import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4FailsafeRows.js" as Rows

/// Tests the pure row-definition and filtering logic behind PX4FailsafesComponent.qml.
TestCase {
    id: testCase
    name: "PX4FailsafeRowsTest"

    readonly property var _allParams: _collectParams(Rows.failsafeRows())

    function _collectParams(rows) {
        var names = {}
        for (var i = 0; i < rows.length; i++) {
            for (var c = 0; c < rows[i].cells.length; c++) {
                var cell = rows[i].cells[c]
                if (cell.param) {
                    names[cell.param] = true
                }
                var bar = cell.thresholdBar || []
                for (var b = 0; b < bar.length; b++) {
                    names[bar[b]] = true
                }
            }
        }
        return names
    }

    function _exists(names) {
        return function(name) { return names[name] === true }
    }

    function _ids(rows) {
        return rows.map(function(row) { return row.id })
    }

    function _rowById(rows, id) {
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].id === id) {
                return rows[i]
            }
        }
        return null
    }

    readonly property var _multiRotor: ({ multiRotor: true, fixedWing: false, vtol: false })
    readonly property var _fixedWing:  ({ multiRotor: false, fixedWing: true, vtol: false })
    readonly property var _vtol:       ({ multiRotor: false, fixedWing: false, vtol: true })

    function test_rowDefinitions_wellFormed() {
        var rows = Rows.failsafeRows()
        var seen = {}
        verify(rows.length > 0)
        for (var i = 0; i < rows.length; i++) {
            var row = rows[i]
            verify(row.id !== undefined && row.id !== "", "row has an id")
            verify(!seen[row.id], "duplicate row id " + row.id)
            seen[row.id] = true
            verify(row.title !== "", row.id + " has a title")
            verify(row.group !== "", row.id + " has a group")
            var paramCells = row.cells.filter(function(cell) { return cell.param || cell.thresholdBar })
            verify(paramCells.length > 0, row.id + " has at least one parameter cell")
        }
    }

    function test_rowDefinitions_groupsAreContiguous() {
        var rows = Rows.failsafeRows()
        var closed = {}
        var current = ""
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].group !== current) {
                verify(!closed[rows[i].group], "group " + rows[i].group + " is split")
                if (current !== "") {
                    closed[current] = true
                }
                current = rows[i].group
            }
        }
    }

    function test_visibleRows_hidesRowsWithoutParameters() {
        var rows = Rows.visibleRows(Rows.failsafeRows(), _multiRotor, _exists({ NAV_DLL_ACT: true, COM_DL_LOSS_T: true }))
        compare(_ids(rows), ["dataLink"])
        compare(rows[0].params, ["COM_DL_LOSS_T", "NAV_DLL_ACT"])
    }

    function test_visibleRows_dropsMissingCellsOnly() {
        var rows = Rows.visibleRows(Rows.failsafeRows(), _multiRotor, _exists({ NAV_DLL_ACT: true }))
        var dataLink = _rowById(rows, "dataLink")
        verify(dataLink)
        compare(dataLink.params, ["NAV_DLL_ACT"])
    }

    function test_visibleRows_allParametersShowsEveryRowForVtol() {
        var all = Rows.failsafeRows()
        var rows = Rows.visibleRows(all, _vtol, _exists(_allParams))
        verify(_rowById(rows, "quadchute"))
        verify(_rowById(rows, "motorFailure"))
        verify(_rowById(rows, "fixedWingGps"))
    }

    function test_visibleRows_vehicleRows_data() {
        return [
            { tag: "quadchuteOnlyOnVtol",      row: "quadchute",     multiRotor: false, fixedWing: false, vtol: true,  shown: true },
            { tag: "quadchuteNotOnMultiRotor", row: "quadchute",     multiRotor: true,  fixedWing: false, vtol: false, shown: false },
            { tag: "quadchuteNotOnFixedWing",  row: "quadchute",     multiRotor: false, fixedWing: true,  vtol: false, shown: false },
            { tag: "fixedWingGpsOnFixedWing",  row: "fixedWingGps",  multiRotor: false, fixedWing: true,  vtol: false, shown: true },
            { tag: "fixedWingGpsNotOnMulti",   row: "fixedWingGps",  multiRotor: true,  fixedWing: false, vtol: false, shown: false },
            { tag: "fixedWingGpsOnVtol",       row: "fixedWingGps",  multiRotor: false, fixedWing: false, vtol: true,  shown: true },
            { tag: "motorFailureOnMultiRotor", row: "motorFailure",  multiRotor: true,  fixedWing: false, vtol: false, shown: true },
            { tag: "motorFailureNotOnFixed",   row: "motorFailure",  multiRotor: false, fixedWing: true,  vtol: false, shown: false },
            { tag: "positionEstimateOnFixedWing", row: "positionEstimate", multiRotor: false, fixedWing: true, vtol: false, shown: true },
            { tag: "positionEstimateOnMultiRotor", row: "positionEstimate", multiRotor: true, fixedWing: false, vtol: false, shown: true },
            { tag: "batteryOnEveryVehicle",    row: "battery",       multiRotor: false, fixedWing: true,  vtol: false, shown: true },
        ]
    }

    function test_visibleRows_vehicleRows(data) {
        var vehicle = { multiRotor: data.multiRotor, fixedWing: data.fixedWing, vtol: data.vtol }
        var rows = Rows.visibleRows(Rows.failsafeRows(), vehicle, _exists(_allParams))
        compare(_rowById(rows, data.row) !== null, data.shown)
    }

    function test_visibleRows_cellVehicleFilter() {
        var positionParams = function(vehicle) {
            return _rowById(Rows.visibleRows(Rows.failsafeRows(), vehicle, _exists(_allParams)), "positionEstimate").params
        }
        verify(positionParams(_multiRotor).indexOf("COM_POS_FS_EPH") >= 0)
        verify(positionParams(_fixedWing).indexOf("COM_POS_FS_EPH") < 0)
        verify(positionParams(_fixedWing).indexOf("COM_VEL_FS_EVH") >= 0)

        var withLoiterRadius = function(vehicle) {
            var row = _rowById(Rows.visibleRows(Rows.failsafeRows(), vehicle, _exists(_allParams)), "returnMode")
            return row.params.indexOf("RTL_LOITER_RAD") >= 0
        }
        verify(!withLoiterRadius(_multiRotor))
        verify(withLoiterRadius(_fixedWing))
        verify(withLoiterRadius(_vtol))
    }

    function test_visibleRows_fixedCellOnlyWhenActionParamMissing() {
        var imbalanced = function(names) {
            return _rowById(Rows.visibleRows(Rows.failsafeRows(), _multiRotor, _exists(names)), "imbalancedPropeller")
        }
        var fixedTexts = function(row) {
            return row.cells.filter(function(cell) { return cell.fixed }).length
        }
        compare(fixedTexts(imbalanced({ FD_IMB_PROP_THR: true })), 1)
        compare(fixedTexts(imbalanced({ FD_IMB_PROP_THR: true, COM_IMB_PROP_ACT: true })), 0)
    }

    function test_visibleRows_thresholdBarNeedsEveryThreshold() {
        var battery = function(names) {
            return _rowById(Rows.visibleRows(Rows.failsafeRows(), _multiRotor, _exists(names)), "battery")
        }
        var hasBar = function(row) {
            return row.cells.some(function(cell) { return cell.thresholdBar })
        }
        verify(hasBar(battery({ COM_LOW_BAT_ACT: true, BAT_LOW_THR: true, BAT_CRIT_THR: true, BAT_EMERGEN_THR: true })))
        verify(!hasBar(battery({ COM_LOW_BAT_ACT: true, BAT_LOW_THR: true, BAT_CRIT_THR: true })))
    }

    function test_isRowActive_data() {
        return [
            { tag: "dataLinkDisabled",       row: "dataLink",     values: { NAV_DLL_ACT: 0 },                        active: false },
            { tag: "dataLinkHold",           row: "dataLink",     values: { NAV_DLL_ACT: 1 },                        active: true },
            { tag: "geofenceNone",           row: "geofence",     values: { GF_ACTION: 0 },                          active: false },
            { tag: "geofenceHold",           row: "geofence",     values: { GF_ACTION: 2 },                          active: true },
            { tag: "maxFlightTimeDisabled",  row: "maxFlightTime", values: { COM_FLT_TIME_MAX: -1 },                 active: false },
            { tag: "maxFlightTimeSet",       row: "maxFlightTime", values: { COM_FLT_TIME_MAX: 1200 },               active: true },
            { tag: "manualControlDisabled",  row: "manualControl", values: { COM_RC_IN_MODE: 4 },                    active: false },
            { tag: "manualControlEnabled",   row: "manualControl", values: { COM_RC_IN_MODE: 3 },                    active: true },
            { tag: "windNeedsThresholdAndAction", row: "highWind", values: { COM_WIND_WARN: -1, COM_WIND_MAX_ACT: 3, COM_WIND_MAX: -1 }, active: false },
            { tag: "windActive",             row: "highWind",     values: { COM_WIND_WARN: -1, COM_WIND_MAX_ACT: 3, COM_WIND_MAX: 15 }, active: true },
            { tag: "windWarningOnly",        row: "highWind",     values: { COM_WIND_WARN: 10, COM_WIND_MAX_ACT: 0, COM_WIND_MAX: -1 }, active: true },
            { tag: "windAllDisabled",         row: "highWind",     values: { COM_WIND_WARN: -1, COM_WIND_MAX_ACT: 0, COM_WIND_MAX: -1 }, active: false },
            { tag: "attitudeBothDisabled",   row: "attitude",     values: { FD_FAIL_R: 0, FD_FAIL_P: 0 },            active: false },
            { tag: "attitudeRollOnly",      row: "attitude",     values: { FD_FAIL_R: 60, FD_FAIL_P: 0 },           active: true },
            { tag: "attitudePitchOnly",     row: "attitude",     values: { FD_FAIL_R: 0, FD_FAIL_P: 60 },           active: true },
            { tag: "windWarnParamMissingDisabled", row: "highWind", values: { COM_WIND_MAX: -1, COM_WIND_MAX_ACT: 0 }, active: false },
            { tag: "windWarnParamMissingEnabled",  row: "highWind", values: { COM_WIND_MAX: 15, COM_WIND_MAX_ACT: 3 }, active: true },
            { tag: "attitudeRollParamMissingDisabled", row: "attitude", values: { FD_FAIL_P: 0 },                    active: false },
            { tag: "attitudeRollParamMissingEnabled",  row: "attitude", values: { FD_FAIL_P: 60 },                   active: true },
            { tag: "gnssNoneRequired",       row: "gnssLoss",     values: { SYS_HAS_NUM_GNSS: 0 },                   active: false },
            { tag: "gnssRequired",           row: "gnssLoss",     values: { SYS_HAS_NUM_GNSS: 1 },                   active: true },
            { tag: "imbalancedDisabledAction", row: "imbalancedPropeller", values: { FD_IMB_PROP_THR: 30, COM_IMB_PROP_ACT: -1 }, active: false },
            { tag: "imbalancedWarning",      row: "imbalancedPropeller", values: { FD_IMB_PROP_THR: 30, COM_IMB_PROP_ACT: 0 },  active: true },
            { tag: "imbalancedActionParamMissing", row: "imbalancedPropeller", values: { FD_IMB_PROP_THR: 30 }, active: true },
            { tag: "manualControlSourceParamMissing", row: "manualControl", values: {}, active: true },
            { tag: "alwaysActiveRow",        row: "battery",      values: {},                                        active: true },
        ]
    }

    function test_isRowActive(data) {
        var row = _rowById(Rows.failsafeRows(), data.row)
        verify(row, "unknown row " + data.row)
        var valueOf = function(name) { return data.values[name] }
        compare(Rows.isRowActive(row, valueOf), data.active)
    }

    readonly property var _severities: ["none", "warning", "fallback", "hold", "return", "land", "terminate"]

    function _cellForParam(param) {
        var rows = Rows.failsafeRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                if (rows[r].cells[c].param === param) {
                    return rows[r].cells[c]
                }
            }
        }
        return null
    }

    function test_actionSeverity_data() {
        return [
            { tag: "rcLossReturn",       param: "NAV_RCL_ACT",      value: 2,  severity: "return" },
            { tag: "rcLossDisarm",       param: "NAV_RCL_ACT",      value: 6,  severity: "terminate" },
            { tag: "dataLinkDisabled",   param: "NAV_DLL_ACT",      value: 0,  severity: "none" },
            { tag: "offboardPosition",   param: "COM_OBL_RC_ACT",   value: 0,  severity: "fallback" },
            { tag: "offboardLand",       param: "COM_OBL_RC_ACT",   value: 4,  severity: "land" },
            { tag: "batteryReturnLand",  param: "COM_LOW_BAT_ACT",  value: 3,  severity: "return" },
            { tag: "geofenceTerminate",  param: "GF_ACTION",        value: 4,  severity: "terminate" },
            { tag: "geofenceLand",       param: "GF_ACTION",        value: 5,  severity: "land" },
            { tag: "windHold",           param: "COM_WIND_MAX_ACT", value: 2,  severity: "hold" },
            { tag: "trafficPositionHold", param: "NAV_TRAFF_AVOID", value: 4,  severity: "hold" },
            { tag: "quadchuteWarning",   param: "COM_QC_ACT",       value: -1, severity: "warning" },
            { tag: "imbalancedDisabled", param: "COM_IMB_PROP_ACT", value: -1, severity: "none" },
            { tag: "unlistedValue",      param: "GF_ACTION",        value: 99, severity: "none" },
            { tag: "missingValue",       param: "GF_ACTION",        value: undefined, severity: "none" },
        ]
    }

    function test_actionSeverity(data) {
        var cell = _cellForParam(data.param)
        verify(cell, "unknown parameter " + data.param)
        compare(Rows.actionSeverity(cell, data.value), data.severity)
    }

    function test_actionSeverity_fixedCellIgnoresText() {
        compare(Rows.actionSeverity({ fixed: "Zurueck", severity: "return" }), "return")
    }

    function test_rowDefinitions_actionsHaveSeverity() {
        var rows = Rows.failsafeRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                var cell = rows[r].cells[c]
                var isActionParam = cell.param && (cell.param.slice(-4) === "_ACT" || cell.param === "NAV_TRAFF_AVOID")
                verify(!isActionParam || cell.severity !== undefined, cell.param + " has no severity")
                if (typeof cell.severity === "string") {
                    verify(_severities.indexOf(cell.severity) >= 0, "bad severity " + cell.severity)
                } else if (cell.severity) {
                    for (var key in cell.severity) {
                        verify(_severities.indexOf(cell.severity[key]) >= 0,
                               rows[r].id + " has bad severity " + cell.severity[key])
                    }
                }
            }
        }
    }

    function test_rowDefinitions_fixedActionsHaveSeverity() {
        var rows = Rows.failsafeRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                var cell = rows[r].cells[c]
                if (cell.fixed !== undefined) {
                    verify(_severities.indexOf(cell.severity) >= 0, rows[r].id + " fixed cell has no severity")
                }
            }
        }
    }
}
