import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as ConfigRows
import "../../../../src/AutoPilotPlugins/PX4/PX4ReturnLandRows.js" as Rows

/// Tests the row definitions and vehicle filtering behind PX4ReturnLandComponent.qml.
TestCase {
    id: testCase
    name: "PX4ReturnLandRowsTest"

    readonly property var _allParams: ConfigRows.allParams(Rows.returnLandRows())

    readonly property var _multiRotor: ({ multiRotor: true, fixedWing: false, vtol: false })
    readonly property var _fixedWing:  ({ multiRotor: false, fixedWing: true, vtol: false })
    readonly property var _vtol:       ({ multiRotor: false, fixedWing: false, vtol: true })

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
        var rows = Rows.returnLandRows()
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

    function test_visibleRows_vehicleRows_data() {
        return [
            { tag: "routeOnMultiRotor",      row: "route",      vehicle: _multiRotor, shown: true },
            { tag: "routeOnFixedWing",       row: "route",      vehicle: _fixedWing,  shown: true },
            { tag: "descentOnMultiRotor",    row: "descent",    vehicle: _multiRotor, shown: true },
            { tag: "descentOnVtol",          row: "descent",    vehicle: _vtol,       shown: true },
            { tag: "descentNotOnFixedWing",  row: "descent",    vehicle: _fixedWing,  shown: false },
            { tag: "touchdownOnFixedWing",   row: "touchdown",  vehicle: _fixedWing,  shown: true },
        ]
    }

    function test_visibleRows_vehicleRows(data) {
        var rows = ConfigRows.visibleRows(Rows.returnLandRows(), data.vehicle, _exists(_allParams))
        compare(_rowById(rows, data.row) !== null, data.shown)
    }

    function test_visibleRows_loiterRadiusOnlyForFixedWingAndVtol_data() {
        return [
            { tag: "multiRotor", vehicle: _multiRotor, shown: false },
            { tag: "fixedWing",  vehicle: _fixedWing,  shown: true },
            { tag: "vtol",       vehicle: _vtol,       shown: true },
        ]
    }

    function test_visibleRows_loiterRadiusOnlyForFixedWingAndVtol(data) {
        var arrival = _rowById(ConfigRows.visibleRows(Rows.returnLandRows(), data.vehicle, _exists(_allParams)), "arrival")
        compare(arrival.params.indexOf("RTL_LOITER_RAD") >= 0, data.shown)
    }

    // Firmware without the slow-landing altitudes still shows the landing descent rate
    function test_visibleRows_dropsMissingParameters() {
        var descent = _rowById(ConfigRows.visibleRows(Rows.returnLandRows(), _multiRotor, _exists(["MPC_LAND_SPEED"])), "descent")
        verify(descent)
        compare(descent.params, ["MPC_LAND_SPEED"])
    }

    function test_visibleRows_hidesRowsWithoutParameters() {
        var rows = ConfigRows.visibleRows(Rows.returnLandRows(), _multiRotor, _exists(["COM_DISARM_LAND"]))
        compare(rows.map(function(row) { return row.id }), ["touchdown"])
    }

    // The firmware rewrites MPC_LAND_SPEED itself while MPC_Z_VEL_ALL is zero or more
    function test_cellOverridden_data() {
        return [
            { tag: "limitOff",     values: { MPC_Z_VEL_ALL: -3 }, overridden: false },
            { tag: "limitZero",    values: { MPC_Z_VEL_ALL: 0 },  overridden: true },
            { tag: "limitOn",      values: { MPC_Z_VEL_ALL: 3 },  overridden: true },
            { tag: "limitMissing", values: {},                    overridden: false },
        ]
    }

    function test_cellOverridden(data) {
        var descent = _rowById(Rows.returnLandRows(), "descent")
        var cellFor = function(param) { return descent.cells.filter(function(cell) { return cell.param === param })[0] }
        var valueOf = function(name) { return data.values[name] }
        compare(Rows.cellOverridden(cellFor("MPC_LAND_SPEED"), valueOf), data.overridden)
        verify(!Rows.cellOverridden(cellFor("MPC_LAND_ALT1"), function() { return 3 }), "other cells are never overridden")
    }
}
