import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4CollisionRows.js" as Rows
import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as ConfigRows

/// Tests the row definitions behind PX4CollisionComponent.qml.
TestCase {
    id: testCase
    name: "PX4CollisionRowsTest"

    readonly property var _multiRotor: ({ multiRotor: true, fixedWing: false, vtol: false })

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
        var rows = Rows.collisionRows()
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

    function test_isRowActive_data() {
        return [
            { tag: "preventionDisabled",     row: "prevention", values: { CP_DIST: -1 }, active: false },
            { tag: "preventionZeroDisables", row: "prevention", values: { CP_DIST: 0 },  active: false },
            { tag: "preventionEnabled",      row: "prevention", values: { CP_DIST: 5 },  active: true },
            { tag: "tuningDisabled",         row: "tuning",     values: { CP_DIST: -1 }, active: false },
            { tag: "tuningEnabled",          row: "tuning",     values: { CP_DIST: 5 },  active: true },
        ]
    }

    function test_isRowActive(data) {
        var row = _rowById(Rows.collisionRows(), data.row)
        verify(row, "unknown row " + data.row)
        compare(ConfigRows.isRowActive(row, function(name) { return data.values[name] }), data.active)
    }

    function test_visibleRows_dropsMissingParameters() {
        var rows = ConfigRows.visibleRows(Rows.collisionRows(), _multiRotor, _exists(["CP_DIST"]))
        compare(rows.map(function(row) { return row.id }), ["prevention"])
        compare(rows[0].params, ["CP_DIST"])
    }
}
