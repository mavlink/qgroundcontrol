import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4BatteryRows.js" as Rows

/// Tests the pure row-definition, filtering and battery source logic behind PX4BatteryComponent.qml.
TestCase {
    id: testCase
    name: "PX4BatteryRowsTest"

    function _exists(names) {
        return function(name) { return names.indexOf(name) >= 0 }
    }

    function _rowById(rows, id) {
        for (var i = 0; i < rows.length; i++) {
            if (rows[i].id === id) {
                return rows[i]
            }
        }
        return null
    }

    function _cellForParam(param) {
        var rows = Rows.batteryRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                if (rows[r].cells[c].param === param) {
                    return rows[r].cells[c]
                }
            }
        }
        return null
    }

    function test_rowDefinitions_wellFormed() {
        var rows = Rows.batteryRows()
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
        var rows = Rows.batteryRows()
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

    function test_rowDefinitions_batteryRowsMatchTheirInstance() {
        for (var instance = 1; instance <= Rows.maxBatteries; instance++) {
            var row = _rowById(Rows.batteryRows(), "battery" + instance)
            verify(row, "no row for battery " + instance)
            compare(row.instance, instance)
            compare(row.sourceParam, "BAT" + instance + "_SOURCE")
            for (var c = 0; c < row.cells.length; c++) {
                verify(row.cells[c].param.indexOf("BAT" + instance + "_") === 0,
                       row.cells[c].param + " is not a battery " + instance + " parameter")
            }
        }
    }

    function test_rowDefinitions_cellInputsAreKnown() {
        var known = ["powerModule", "external", "canFiltered", "escs", "idleOverwrite"]
        var rows = Rows.batteryRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                var inputs = rows[r].cells[c].inputs || []
                for (var i = 0; i < inputs.length; i++) {
                    verify(known.indexOf(inputs[i]) >= 0, rows[r].id + " has unknown input " + inputs[i])
                }
            }
        }
    }

    function test_rowDefinitions_calculatorsOnlyOnAnalogCells() {
        var rows = Rows.batteryRows()
        for (var r = 0; r < rows.length; r++) {
            for (var c = 0; c < rows[r].cells.length; c++) {
                var cell = rows[r].cells[c]
                if (cell.calc !== undefined) {
                    verify(["voltageDivider", "ampsPerVolt"].indexOf(cell.calc) >= 0, "bad calculator " + cell.calc)
                    compare(cell.inputs, ["powerModule"])
                }
            }
        }
    }

    // BAT3 has no analog parameters, and PX4 main replaced BAT#_I_CHANNEL with BAT#_V_FILT / BAT#_I_FILT
    function test_visibleRows_keepsOnlyParametersOnTheVehicle() {
        var params = ["BAT1_SOURCE", "BAT1_N_CELLS", "BAT1_V_DIV", "BAT1_V_FILT", "BAT3_SOURCE", "BAT3_N_CELLS"]
        var rows = Rows.visibleRows(Rows.batteryRows(), _exists(params))
        compare(rows.map(function(row) { return row.id }), ["battery1", "battery3"])
        compare(_rowById(rows, "battery1").params, ["BAT1_SOURCE", "BAT1_N_CELLS", "BAT1_V_DIV", "BAT1_V_FILT"])
        compare(_rowById(rows, "battery3").instance, 3)
    }

    function test_visibleRows_thresholdBarNeedsEveryLevel() {
        var levels = function(params) {
            return _rowById(Rows.visibleRows(Rows.batteryRows(), _exists(params)), "levels")
        }
        var hasBar = function(row) {
            return row.cells.some(function(cell) { return cell.thresholdBar })
        }
        verify(hasBar(levels(["BAT_LOW_THR", "BAT_CRIT_THR", "BAT_EMERGEN_THR"])))
        verify(!hasBar(levels(["BAT_LOW_THR", "BAT_CRIT_THR"])))
    }

    function test_batteryInputs_data() {
        return [
            { tag: "disabled",          source: -1,        can: 0,         canEnabled: 2, overwrite: false, inputs: [] },
            { tag: "powerModule",       source: 0,         can: 0,         canEnabled: 2, overwrite: false, inputs: ["powerModule", "idleOverwrite"] },
            { tag: "external",          source: 1,         can: 0,         canEnabled: 2, overwrite: true,  inputs: ["external"] },
            { tag: "externalCanRaw",    source: 1,         can: 1,         canEnabled: 2, overwrite: true,  inputs: ["external"] },
            { tag: "externalCanFilter", source: 1,         can: 2,         canEnabled: 2, overwrite: false, inputs: ["external", "canFiltered"] },
            { tag: "externalNoCanParam", source: 1,        can: undefined, canEnabled: undefined, overwrite: false, inputs: ["external"] },
            { tag: "externalCanFilterDroneCanOff", source: 1, can: 2,      canEnabled: 0, overwrite: true,  inputs: ["external"] },
            { tag: "powerModuleCanFilter", source: 0,      can: 2,         canEnabled: 2, overwrite: false, inputs: ["powerModule", "idleOverwrite"] },
            // The DroneCAN bridge runs the estimate without checking BAT#_SOURCE
            { tag: "disabledCanFilter", source: -1,        can: 2,         canEnabled: 2, overwrite: false, inputs: ["canFiltered"] },
            { tag: "disabledCanFilterDroneCanOff", source: -1, can: 2,     canEnabled: 0, overwrite: true,  inputs: [] },
            { tag: "escs",              source: 2,         can: 0,         canEnabled: 2, overwrite: false, inputs: ["escs"] },
            { tag: "unknownSource",     source: 7,         can: 0,         canEnabled: 2, overwrite: true,  inputs: [] },
            { tag: "missingSource",     source: undefined, can: 0,         canEnabled: 2, overwrite: true,  inputs: [] },
            // BAT#_I_OVERWRITE reaches ESC and filtered DroneCAN batteries only once the estimate applies it
            { tag: "escsOverwrite",     source: 2,         can: 0,         canEnabled: 2, overwrite: true,  inputs: ["escs", "idleOverwrite"] },
            { tag: "externalCanFilterOverwrite", source: 1, can: 2,        canEnabled: 2, overwrite: true,  inputs: ["external", "canFiltered", "idleOverwrite"] },
            { tag: "disabledCanFilterOverwrite", source: -1, can: 2,       canEnabled: 2, overwrite: true,  inputs: ["canFiltered", "idleOverwrite"] },
        ]
    }

    function test_batteryInputs(data) {
        compare(Rows.batteryInputs(data.source, data.can, data.canEnabled, data.overwrite), data.inputs)
    }

    function test_overwriteInEstimator_data() {
        return [
            { tag: "v1_17",       major: 1,         minor: 17,        applied: false },
            { tag: "v1_18",       major: 1,         minor: 18,        applied: true },
            { tag: "v1_19",       major: 1,         minor: 19,        applied: true },
            { tag: "v2_0",        major: 2,         minor: 0,         applied: true },
            { tag: "v0_9",        major: 0,         minor: 9,         applied: false },
            { tag: "unknown",     major: -1,        minor: -1,        applied: true },
            { tag: "undefined",   major: undefined, minor: undefined, applied: true },
        ]
    }

    function test_overwriteInEstimator(data) {
        compare(Rows.overwriteInEstimator(data.major, data.minor), data.applied)
    }

    function test_cellUsed_data() {
        return [
            { tag: "sourceAlwaysShown",         param: "BAT1_SOURCE",      inputs: [],                          used: true },
            { tag: "cellsPowerModule",          param: "BAT1_N_CELLS",     inputs: ["powerModule"],             used: true },
            { tag: "cellsEscs",                 param: "BAT1_N_CELLS",     inputs: ["escs"],                    used: true },
            { tag: "cellsExternalUnused",       param: "BAT1_N_CELLS",     inputs: ["external"],                used: false },
            { tag: "cellsCanFiltered",          param: "BAT1_N_CELLS",     inputs: ["external", "canFiltered"], used: true },
            { tag: "cellsDisabledUnused",       param: "BAT1_N_CELLS",     inputs: [],                          used: false },
            { tag: "cellsDisabledCanFiltered",  param: "BAT1_N_CELLS",     inputs: ["canFiltered"],             used: true },
            { tag: "capacityCanFiltered",       param: "BAT2_CAPACITY",    inputs: ["external", "canFiltered"], used: true },
            { tag: "dividerPowerModule",        param: "BAT1_V_DIV",       inputs: ["powerModule"],             used: true },
            { tag: "dividerEscsUnused",         param: "BAT1_V_DIV",       inputs: ["escs"],                    used: false },
            { tag: "dividerCanFilteredUnused",  param: "BAT1_V_DIV",       inputs: ["external", "canFiltered"], used: false },
            { tag: "idleCurrentApplied",        param: "BAT1_I_OVERWRITE", inputs: ["escs", "idleOverwrite"], used: true },
            { tag: "idleCurrentNotApplied",     param: "BAT1_I_OVERWRITE", inputs: ["escs"],                    used: false },
            { tag: "idleCurrentExternalUnused", param: "BAT1_I_OVERWRITE", inputs: ["external"],                used: false },
        ]
    }

    function test_cellUsed(data) {
        var cell = _cellForParam(data.param)
        verify(cell, "unknown parameter " + data.param)
        compare(Rows.cellUsed(cell, data.inputs), data.used)
    }

    function test_allParams_listsEveryParameterOnce() {
        var params = Rows.allParams(Rows.batteryRows())
        var expected = ["BAT1_SOURCE", "BAT3_N_CELLS", "BAT2_V_FILT", "BAT_LOW_THR", "BAT_EMERGEN_THR",
                        "INA226_SHUNT", "SENS_EN_INA220", "VOXLPM_SHUNT_BAT", "UAVCAN_SUB_BAT"]
        for (var i = 0; i < expected.length; i++) {
            verify(params.indexOf(expected[i]) >= 0, expected[i] + " not listed")
        }
        compare(params.filter(function(name, index) { return params.indexOf(name) !== index }), [])
    }

    function test_usedParams_onlyListsCellsTheSourceUses() {
        var params = ["BAT1_SOURCE", "BAT1_N_CELLS", "BAT1_V_DIV", "BAT_LOW_THR", "BAT_CRIT_THR", "BAT_EMERGEN_THR"]
        var rows = Rows.visibleRows(Rows.batteryRows(), _exists(params))
        var battery = _rowById(rows, "battery1")
        compare(Rows.usedParams(battery, Rows.batteryInputs(1, 0)), ["BAT1_SOURCE"])
        compare(Rows.usedParams(battery, Rows.batteryInputs(2, 0)), ["BAT1_SOURCE", "BAT1_N_CELLS"])
        compare(Rows.usedParams(battery, Rows.batteryInputs(0, 0)), ["BAT1_SOURCE", "BAT1_N_CELLS", "BAT1_V_DIV"])
        compare(Rows.usedParams(_rowById(rows, "levels"), []), ["BAT_EMERGEN_THR", "BAT_CRIT_THR", "BAT_LOW_THR"])
    }

    function test_isRowActive_data() {
        return [
            { tag: "batteryDisabled",       row: "battery1", values: { BAT1_SOURCE: -1 },                      active: false },
            { tag: "batteryPowerModule",    row: "battery1", values: { BAT1_SOURCE: 0 },                       active: true },
            { tag: "batteryExternal",       row: "battery2", values: { BAT2_SOURCE: 1 },                       active: true },
            { tag: "batteryDisabledCanFilter", row: "battery1", values: { BAT1_SOURCE: -1, UAVCAN_ENABLE: 2, UAVCAN_SUB_BAT: 2 }, active: true },
            { tag: "batteryDisabledCanRaw", row: "battery1", values: { BAT1_SOURCE: -1, UAVCAN_ENABLE: 2, UAVCAN_SUB_BAT: 1 }, active: false },
            { tag: "batteryDisabledCanOff", row: "battery1", values: { BAT1_SOURCE: -1, UAVCAN_ENABLE: 0, UAVCAN_SUB_BAT: 2 }, active: false },
            { tag: "ina226Disabled",        row: "ina226",   values: { SENS_EN_INA226: 0 },                    active: false },
            { tag: "ina226Enabled",         row: "ina226",   values: { SENS_EN_INA226: 1 },                    active: true },
            // Some boards start the monitors without setting an enable flag
            { tag: "ina226AutoStarted",     row: "ina226",   values: { SENS_EN_INA226: 0, BAT1_SOURCE: 0 },    active: true },
            { tag: "ina238AutoStartedBat3", row: "ina238",   values: { SENS_EN_INA238: 0, BAT1_SOURCE: -1, BAT3_SOURCE: 0 }, active: true },
            { tag: "ina226NoPowerModule",   row: "ina226",   values: { SENS_EN_INA226: 0, BAT1_SOURCE: 1, BAT2_SOURCE: 2 }, active: false },
            { tag: "canSubscriptionOff",    row: "droneCan", values: { UAVCAN_ENABLE: 2, UAVCAN_SUB_BAT: 0 },  active: false },
            { tag: "canDisabled",           row: "droneCan", values: { UAVCAN_ENABLE: 0, UAVCAN_SUB_BAT: 1 },  active: false },
            { tag: "canSubscribed",         row: "droneCan", values: { UAVCAN_ENABLE: 2, UAVCAN_SUB_BAT: 2 },  active: true },
            { tag: "sharedLevelsAlways",    row: "levels",   values: {},                                       active: true },
        ]
    }

    function test_isRowActive(data) {
        var row = _rowById(Rows.batteryRows(), data.row)
        verify(row, "unknown row " + data.row)
        compare(Rows.isRowActive(row, function(name) { return data.values[name] }), data.active)
    }

    // PX4 sends a battery it estimates itself as MAVLink id instance - 1. DroneCAN batteries use their node id - 1,
    // and MAVLink batteries 255, so an External battery row cannot claim telemetry.
    function test_telemetryId_data() {
        return [
            { tag: "powerModule",       instance: 1, source: 0,  can: 0, id: 0 },
            { tag: "powerModuleBat3",   instance: 3, source: 0,  can: 0, id: 2 },
            { tag: "escs",              instance: 1, source: 2,  can: 0, id: 0 },
            // PX4 only runs the ESC battery as battery 1
            { tag: "escsOnBattery2",    instance: 2, source: 2,  can: 0, id: -1 },
            { tag: "external",          instance: 2, source: 1,  can: 0, id: -1 },
            { tag: "externalCanFilter", instance: 2, source: 1,  can: 2, id: -1 },
            { tag: "disabled",          instance: 1, source: -1, can: 0, id: -1 },
            { tag: "disabledCanFilter", instance: 1, source: -1, can: 2, id: -1 },
        ]
    }

    function test_telemetryId(data) {
        compare(Rows.telemetryId(data.instance, Rows.batteryInputs(data.source, data.can, 2)), data.id)
    }

    function test_unclaimedTelemetryIds_data() {
        var powerModule1 = { instance: 1, inputs: Rows.batteryInputs(0, 0) }
        var external2 = { instance: 2, inputs: Rows.batteryInputs(1, 0) }
        return [
            { tag: "claimedByPowerModule",   reported: [0],         batteries: [powerModule1],            unclaimed: [] },
            { tag: "unmatchedDroneCanNode",  reported: [0, 41],     batteries: [powerModule1, external2], unclaimed: [41] },
            // DroneCAN node 2 arrives as id 1, which battery 2 would claim if it were a power module
            { tag: "droneCanOverlapsRowId",  reported: [1],         batteries: [powerModule1, external2], unclaimed: [1] },
            { tag: "mavlinkExternal",        reported: [255],       batteries: [powerModule1, external2], unclaimed: [255] },
            { tag: "noBatteryRows",          reported: [0, 1],      batteries: [],                        unclaimed: [0, 1] },
        ]
    }

    function test_unclaimedTelemetryIds(data) {
        compare(Rows.unclaimedTelemetryIds(data.reported, data.batteries), data.unclaimed)
    }

    // PX4 only runs the ESC battery on battery 1
    function test_sourceWarning_data() {
        return [
            { tag: "battery1Escs",        instance: 1, source: 2, warned: false },
            { tag: "battery2Escs",        instance: 2, source: 2, warned: true },
            { tag: "battery3Escs",        instance: 3, source: 2, warned: true },
            { tag: "battery2PowerModule", instance: 2, source: 0, warned: false },
        ]
    }

    function test_sourceWarning(data) {
        compare(Rows.sourceWarning(data.instance, data.source) !== "", data.warned)
    }
}
