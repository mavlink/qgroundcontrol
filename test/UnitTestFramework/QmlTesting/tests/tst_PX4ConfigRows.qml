import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as Rows

/// Tests the logic shared by the row-based PX4 setup pages.
TestCase {
    name: "PX4ConfigRowsTest"

    // A delegate gets a copy of its row, not the object in the group
    function _copy(row) {
        return JSON.parse(JSON.stringify(row))
    }

    function test_hasShownRowAbove() {
        var rows = [{ id: "a" }, { id: "b" }, { id: "c" }]
        var shownIds = ["b", "c"]
        var shown = function(row) { return shownIds.indexOf(row.id) >= 0 }

        // The first shown row sits under the group heading's rule, so it needs none of its own
        verify(!Rows.hasShownRowAbove(rows, _copy(rows[0]), shown))
        verify(!Rows.hasShownRowAbove(rows, _copy(rows[1]), shown))
        verify(Rows.hasShownRowAbove(rows, _copy(rows[2]), shown))

        shownIds = ["a", "b", "c"]
        verify(!Rows.hasShownRowAbove(rows, _copy(rows[0]), shown))
        verify(Rows.hasShownRowAbove(rows, _copy(rows[1]), shown))
        verify(Rows.hasShownRowAbove(rows, _copy(rows[2]), shown))

        verify(!Rows.hasShownRowAbove(rows, { id: "missing" }, shown))
    }
}
