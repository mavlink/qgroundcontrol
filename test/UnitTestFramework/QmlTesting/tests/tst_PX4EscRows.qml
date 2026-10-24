import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as ConfigRows
import "../../../../src/AutoPilotPlugins/PX4/PX4EscRows.js" as Rows

/// Tests the row definitions and output protocol logic behind PX4EscComponent.qml.
TestCase {
    id: testCase
    name: "PX4EscRowsTest"

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

    function test_rowDefinitions_wellFormed() {
        var rows = Rows.escRows(4)
        var seen = {}
        verify(rows.length > 0)
        for (var i = 0; i < rows.length; i++) {
            var row = rows[i]
            verify(row.id !== undefined && row.id !== "", "row has an id")
            verify(!seen[row.id], "duplicate row id " + row.id)
            seen[row.id] = true
            verify(row.title !== "", row.id + " has a title")
            verify(row.group !== "", row.id + " has a group")
            verify(row.cells.some(function(cell) { return cell.param }), row.id + " has a parameter cell")
        }
    }

    function test_timerParams_coverMainAndAux() {
        var names = Rows.timerParams()
        verify(names.indexOf("PWM_MAIN_TIM0") >= 0)
        verify(names.indexOf("PWM_AUX_TIM0") >= 0)
        compare(names.length, new Set(names).size)
    }

    function test_outputKinds_data() {
        return [
            { tag: "noProtocols",     protocols: [],                 known: false, pwm: false, dshot: false },
            { tag: "pwmRate",         protocols: [400],              known: true,  pwm: true,  dshot: false },
            { tag: "oneShot",         protocols: [-1],               known: true,  pwm: true,  dshot: false },
            { tag: "dshot",           protocols: [-4],               known: true,  pwm: false, dshot: true },
            { tag: "bidirectionalDshot", protocols: [-7],            known: true,  pwm: false, dshot: true },
            { tag: "mixedGroups",     protocols: [-3, 400],          known: true,  pwm: true,  dshot: true },
        ]
    }

    function test_outputKinds(data) {
        var kinds = Rows.outputKinds(data.protocols)
        compare(kinds.known, data.known)
        compare(kinds.pwm, data.pwm)
        compare(kinds.dshot, data.dshot)
    }

    function test_calibrationShown_data() {
        return [
            { tag: "unknownOutputs",  protocols: [],        shown: true },
            { tag: "pwmOutputs",      protocols: [400],     shown: true },
            { tag: "dshotOnly",       protocols: [-4, -5],  shown: false },
            { tag: "pwmAndDshot",     protocols: [-4, 400], shown: true },
        ]
    }

    function test_calibrationShown(data) {
        compare(Rows.calibrationShown(Rows.outputKinds(data.protocols)), data.shown)
    }

    function test_rowShown_dshotRowsNeedDshotOutput() {
        var output = _rowById(Rows.escRows(4), "dshotOutput")
        var canRow = _rowById(Rows.escRows(4), "dronecanEsc")
        verify(output)
        verify(!Rows.rowShown(output, Rows.outputKinds([400])))
        verify(Rows.rowShown(output, Rows.outputKinds([-4])))
        verify(Rows.rowShown(canRow, Rows.outputKinds([400])))
    }

    function test_cellShown_deadbandNeeds3dMode() {
        var output = _rowById(Rows.escRows(4), "dshotOutput")
        var deadband = output.cells.filter(function(cell) { return cell.param === "DSHOT_3D_DEAD_L" })[0]
        verify(deadband)
        verify(!Rows.cellShown(deadband, function() { return 0 }))
        verify(Rows.cellShown(deadband, function() { return 1 }))
        verify(!Rows.cellShown(deadband, function() { return undefined }))
        verify(Rows.cellShown(output.cells[0], function() { return undefined }))
    }

    function test_visibleRows_firmwareParameterSets_data() {
        return [
            { tag: "firmware117", names: ["DSHOT_MIN", "DSHOT_BIDIR_EN", "MOT_POLE_COUNT", "UAVCAN_ESC_IFACE"],
              telemetry: ["DSHOT_BIDIR_EN", "MOT_POLE_COUNT"] },
            { tag: "mainPerMotorPoles", names: ["DSHOT_MIN", "DSHOT_BIDIR_EDT", "DSHOT_ESC_TYPE", "DSHOT_MOT_POL1", "DSHOT_MOT_POL2"],
              telemetry: ["DSHOT_BIDIR_EDT", "DSHOT_ESC_TYPE", "DSHOT_MOT_POL1", "DSHOT_MOT_POL2"] },
        ]
    }

    function test_visibleRows_firmwareParameterSets(data) {
        var row = _rowById(ConfigRows.visibleRows(Rows.escRows(12), ({}), _exists(data.names)), "dshotTelemetry")
        verify(row, "telemetry row is hidden")
        compare(row.params, data.telemetry)
    }

    function test_escRows_poleCellsFollowMotorCount() {
        var poleParams = function(motorCount) {
            return _rowById(Rows.escRows(motorCount), "dshotTelemetry").cells
                .filter(function(cell) { return cell.param.indexOf("DSHOT_MOT_POL") === 0 })
                .map(function(cell) { return cell.param })
        }
        compare(poleParams(4), ["DSHOT_MOT_POL1", "DSHOT_MOT_POL2", "DSHOT_MOT_POL3", "DSHOT_MOT_POL4"])
        compare(poleParams(0), [])
    }

    function test_visibleRows_hidesRowsWithoutParameters() {
        var rows = ConfigRows.visibleRows(Rows.escRows(4), ({}), _exists(["UAVCAN_ESC_IFACE"]))
        compare(rows.map(function(row) { return row.id }), ["dronecanEsc"])
    }
}
