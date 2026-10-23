import QtQuick
import QtTest

import "../../../../src/AutoPilotPlugins/PX4/PX4ConfigRows.js" as ConfigRows
import "../../../../src/AutoPilotPlugins/PX4/PX4FlightModeRows.js" as Rows

/// Tests the pure row-definition and RC logic behind PX4FlightModes.qml.
TestCase {
    name: "PX4FlightModeRowsTest"

    function test_rowDefinitions_wellFormed() {
        var rows = Rows.flightModeRows()
        var seen = {}
        verify(rows.length > 0)
        for (var i = 0; i < rows.length; i++) {
            var row = rows[i]
            verify(row.id !== undefined && row.id !== "", "row has an id")
            verify(!seen[row.id], "duplicate row id " + row.id)
            seen[row.id] = true
            verify(row.title !== "", row.id + " has a title")
            verify(row.cells.length > 0, row.id + " has cells")
            for (var c = 0; c < row.cells.length; c++) {
                verify(row.cells[c].label !== undefined && row.cells[c].label !== "", row.id + " cell has a label")
            }
        }
    }

    function test_rowDefinitions_groupsAreContiguous() {
        var names = ConfigRows.groupRows(Rows.flightModeRows()).map(function (group) { return group.name })
        for (var i = 0; i < names.length; i++) {
            compare(names.indexOf(names[i]), i, "group " + names[i] + " is split")
        }
    }

    function test_rowDefinitions_switchesShowTheirChannel() {
        var rows = Rows.flightModeRows().filter(function (row) { return row.kind === "switch" })
        verify(rows.length > 0)
        for (var i = 0; i < rows.length; i++) {
            var row = rows[i]
            compare(row.cells[0].param, row.channelParam, row.id + " shows its channel first")
            compare(row.activeWhen[0].param, row.channelParam, row.id + " is active when assigned")
            verify(row.thresholdParam || row.fixedThreshold !== undefined || row.id === "flaps",
                   row.id + " has a threshold")
        }
    }

    // PX4 1.17 uses COM_RC_ARM_HYST for a momentary arm button too, so the Arm row shows it even with the gesture off
    function test_armRowShowsHoldTime() {
        var row = Rows.flightModeRows().filter(function (candidate) { return candidate.id === "arm" })[0]
        verify(row.cells.some(function (cell) { return cell.param === "COM_RC_ARM_HYST" }))
    }

    // Stick override needs COM_RC_OVERRIDE set and a stick speed that is not negative. A speed of 0 still takes over
    // (PX4 ManualControl.cpp).
    function test_isRowActive_stickOverride_data() {
        return [
            { tag: "speed zero",          values: { COM_RC_OVERRIDE: 1, MAN_OVERRIDE_SPD: 0 },  active: true },
            { tag: "speed negative",      values: { COM_RC_OVERRIDE: 1, MAN_OVERRIDE_SPD: -1 }, active: false },
            { tag: "override off",        values: { COM_RC_OVERRIDE: 0, MAN_OVERRIDE_SPD: 1 },  active: false },
            { tag: "speed param missing", values: { COM_RC_OVERRIDE: 1 },                       active: true }
        ]
    }

    function test_isRowActive_stickOverride(data) {
        var row = Rows.flightModeRows().filter(function (candidate) { return candidate.id === "stickOverride" })[0]
        compare(ConfigRows.isRowActive(row, function (name) { return data.values[name] }), data.active)
    }

    // The Assigned filter hides unassigned switches but always shows the mode selection rows. Decided from the row
    // itself, because a row read back from a Repeater model is not the object the page filtered.
    function test_isOptionalRow() {
        var rows = Rows.flightModeRows()
        var byId = {}
        rows.forEach(function (row) { byId[row.id] = row })
        verify(ConfigRows.isOptionalRow(byId["return"]))
        verify(ConfigRows.isOptionalRow(byId["slowKnobs"]))
        verify(!ConfigRows.isOptionalRow(byId["slots"]))
        verify(!ConfigRows.isOptionalRow(byId["slowLimits"]))
        // A copy of the row, as a Repeater hands it to its delegate
        verify(ConfigRows.isOptionalRow(JSON.parse(JSON.stringify(byId["loiter"]))))
    }

    function test_normalizedChannel_data() {
        return [
            { tag: "at trim",           pwm: 1500, reversed: false, expected: 0 },
            { tag: "at min",            pwm: 1000, reversed: false, expected: -1 },
            { tag: "at max",            pwm: 2000, reversed: false, expected: 1 },
            { tag: "below min clamps",  pwm: 900,  reversed: false, expected: -1 },
            { tag: "above max clamps",  pwm: 2100, reversed: false, expected: 1 },
            { tag: "low half",          pwm: 1250, reversed: false, expected: -0.5 },
            { tag: "high half",         pwm: 1750, reversed: false, expected: 0.5 },
            { tag: "reversed",          pwm: 1750, reversed: true,  expected: -0.5 }
        ]
    }

    function test_normalizedChannel(data) {
        fuzzyCompare(Rows.normalizedChannel(data.pwm, 1000, 1500, 2000, data.reversed), data.expected, 1e-6)
    }

    function test_normalizedChannel_asymmetricTrim() {
        fuzzyCompare(Rows.normalizedChannel(1200, 1000, 1100, 2000, false), 100 / 900, 1e-6)
        fuzzyCompare(Rows.normalizedChannel(1050, 1000, 1100, 2000, false), -0.5, 1e-6)
    }

    function test_modeSlot_data() {
        return [
            { tag: "channel min",   value: -1,  expected: 1 },
            { tag: "channel max",   value: 1,   expected: 6 },
            { tag: "past max",      value: 2,   expected: 6 },
            { tag: "center",        value: 0,   expected: 4 }
        ]
    }

    function test_modeSlot(data) {
        compare(Rows.modeSlot(data.value), data.expected)
    }

    // Each boundary is where the next slot starts
    function test_modeSlot_changesAtEachBoundary() {
        var bounds = Rows.slotBoundaries()
        compare(bounds.length, Rows.modeSlotCount - 1)
        for (var i = 0; i < bounds.length; i++) {
            compare(Rows.modeSlot(bounds[i] - 1e-4), i + 1, "below boundary " + i)
            compare(Rows.modeSlot(bounds[i] + 1e-4), i + 2, "above boundary " + i)
        }
    }

    // The live strip draws the slots at equal widths, so a channel value sits inside its own slot's segment
    function test_slotStripPosition() {
        var bounds = Rows.slotBoundaries()
        fuzzyCompare(Rows.slotStripPosition(-1), 0, 1e-6)
        fuzzyCompare(Rows.slotStripPosition(1), 1, 1e-6)
        for (var i = 0; i < bounds.length; i++) {
            fuzzyCompare(Rows.slotStripPosition(bounds[i]), (i + 1) / Rows.modeSlotCount, 1e-6, "boundary " + i)
        }
        for (var value = -1; value <= 1; value += 0.05) {
            var position = Rows.slotStripPosition(value)
            var segment = Math.min(Rows.modeSlotCount, Math.floor(position * Rows.modeSlotCount) + 1)
            compare(segment, Rows.modeSlot(value), "value " + value)
        }
    }

    function test_slotPwmRanges_data() {
        return [
            { tag: "normal",            min: 1000, trim: 1500, max: 2000, reversed: false },
            { tag: "reversed",          min: 1000, trim: 1500, max: 2000, reversed: true },
            { tag: "asymmetric trim",   min: 988,  trim: 1100, max: 2012, reversed: false }
        ]
    }

    // A PWM inside each slot's range selects that slot, and the ranges cover the channel without gaps. The shown
    // endpoints are inclusive, so each selects its own slot and the next µs out selects a neighbour.
    function test_slotPwmRanges(data) {
        function slotAt(pwm) {
            return Rows.modeSlot(Rows.normalizedChannel(pwm, data.min, data.trim, data.max, data.reversed))
        }
        var ranges = Rows.slotPwmRanges(data.min, data.trim, data.max, data.reversed)
        compare(ranges.length, Rows.modeSlotCount)
        for (var slot = 1; slot <= ranges.length; slot++) {
            var range = ranges[slot - 1]
            var low = range.low === null ? data.min : range.low
            var high = range.high === null ? data.max : range.high
            verify(low < high, "slot " + slot + " range is ordered")
            compare(slotAt((low + high) / 2), slot, "slot " + slot + " middle")
            if (range.low !== null) {
                compare(slotAt(range.low), slot, "slot " + slot + " low end")
                verify(slotAt(range.low - 1) !== slot, "slot " + slot + " below low end")
            }
            if (range.high !== null) {
                compare(slotAt(range.high), slot, "slot " + slot + " high end")
                verify(slotAt(range.high + 1) !== slot, "slot " + slot + " above high end")
            }
        }
        var openLow = data.reversed ? ranges[Rows.modeSlotCount - 1] : ranges[0]
        var openHigh = data.reversed ? ranges[0] : ranges[Rows.modeSlotCount - 1]
        compare(openLow.low, null)
        compare(openHigh.high, null)
    }

    function test_switchPosition_data() {
        return [
            { tag: "above threshold",           value: 0.8,  on: 0.75,  mid: undefined, expected: "on" },
            { tag: "below threshold",           value: 0.2,  on: 0.75,  mid: undefined, expected: "off" },
            { tag: "inverted, low channel",     value: -1,   on: -0.25, mid: undefined, expected: "on" },
            { tag: "inverted, high channel",    value: 1,    on: -0.25, mid: undefined, expected: "off" },
            { tag: "three position on",         value: 1,    on: 0.75,  mid: 0.25,      expected: "on" },
            { tag: "three position mid",        value: 0,    on: 0.75,  mid: 0.25,      expected: "mid" },
            { tag: "three position off",        value: -1,   on: 0.75,  mid: 0.25,      expected: "off" }
        ]
    }

    function test_switchPosition(data) {
        compare(Rows.switchPosition(data.value, data.on, data.mid), data.expected)
    }

    function test_buttonChannels() {
        compare(Rows.buttonChannels(0), [])
        compare(Rows.buttonChannels((1 << 6) | (1 << 8)), [7, 9])
        // PX4 uses at most six buttons
        compare(Rows.buttonChannels((1 << 18) - 1), [1, 2, 3, 4, 5, 6])
        compare(Rows.ignoredButtonChannels((1 << 12) - 1), [7, 8, 9, 10, 11, 12])
        compare(Rows.ignoredButtonChannels((1 << 6) | (1 << 8)), [])
    }

    function test_buttonSlot() {
        var mask = (1 << 6) | (1 << 7)
        var values = { 7: -1, 8: 1 }
        compare(Rows.buttonSlot(mask, function (channel) { return values[channel] }), 2)
        values[8] = -1
        compare(Rows.buttonSlot(mask, function (channel) { return values[channel] }), 0)
        compare(Rows.buttonSlot(mask, function (channel) { return undefined }), 0)
    }

    // A released mode button leaves its mode selected, so the page keeps showing its slot until another button is
    // pressed or RC input stops
    function test_nextButtonSlot_data() {
        return [
            { tag: "press",           previous: 0, values: { 7: 1, 8: -1 },  expected: 1 },
            { tag: "release keeps",   previous: 1, values: { 7: -1, 8: -1 }, expected: 1 },
            { tag: "other button",    previous: 1, values: { 7: -1, 8: 1 },  expected: 2 },
            { tag: "no input clears", previous: 1, values: {},               expected: 0 }
        ]
    }

    function test_nextButtonSlot(data) {
        var mask = (1 << 6) | (1 << 7)
        compare(Rows.nextButtonSlot(data.previous, mask, function (channel) { return data.values[channel] }), data.expected)
    }

    function test_selectionMethod_data() {
        return [
            { tag: "channel",               channel: 5, mask: 0,  expected: "switch" },
            { tag: "channel wins",          channel: 5, mask: 64, expected: "switch" },
            { tag: "buttons",               channel: 0, mask: 64, expected: "buttons" },
            { tag: "none",                  channel: 0, mask: 0,  expected: "none" }
        ]
    }

    function test_selectionMethod(data) {
        compare(Rows.selectionMethod(data.channel, data.mask), data.expected)
    }

    // Choosing a method clears what the others use, so PX4 is left with only the chosen one
    function test_selectionWrites_data() {
        return [
            { tag: "switch clears buttons",     method: "switch",  channel: 5, mask: 64,
              expected: [{ param: "RC_MAP_FLTM_BTN", value: 0 }] },
            { tag: "buttons clears channel",    method: "buttons", channel: 5, mask: 64,
              expected: [{ param: "RC_MAP_FLTMODE", value: 0 }] },
            { tag: "none clears both",          method: "none",    channel: 5, mask: 64,
              expected: [{ param: "RC_MAP_FLTMODE", value: 0 }, { param: "RC_MAP_FLTM_BTN", value: 0 }] },
            { tag: "nothing to clear",          method: "switch",  channel: 0, mask: 0, expected: [] }
        ]
    }

    function test_selectionWrites(data) {
        compare(Rows.selectionWrites(data.method, data.channel, data.mask), data.expected)
    }

    // A switch is on over part of the channel range: above a positive threshold, below a negative one
    function test_switchOnRange_data() {
        return [
            { tag: "on above",              on: 0.75,  mid: undefined, low: 0.75, high: 1 },
            { tag: "on below",              on: -0.25, mid: undefined, low: 0,    high: 0.25 },
            { tag: "three position",        on: 0.75,  mid: 0.25,      low: 0.25, high: 1 },
            { tag: "three position below",  on: -0.25, mid: -0.75,     low: 0,    high: 0.75 },
            // Mixed signs leave one part impossible, so only the other part counts
            { tag: "on above, mid inverted", on: 0.75, mid: -0.25,     low: 0.75, high: 1 },
            { tag: "on inverted, mid above", on: -0.25, mid: 0.75,     low: 0.75, high: 1 },
            { tag: "never on",              on: -0.25, mid: 1,         low: 1,    high: 0 }
        ]
    }

    function test_switchOnRange(data) {
        var range = Rows.switchOnRange(data.on, data.mid)
        fuzzyCompare(range.low, data.low, 1e-6)
        fuzzyCompare(range.high, data.high, 1e-6)
    }

    // Functions sharing a channel only conflict when they can be on at the same channel position. Assignments
    // without a range use the whole channel, like sticks and the mode channel.
    function test_channelConflicts_data() {
        var high = { low: 0.75, high: 1 }
        var low = { low: 0, high: 0.25 }
        var upperHalf = { low: 0.5, high: 1 }
        var lowerHalf = { low: 0, high: 0.5 }
        return [
            { tag: "same threshold", assignments: [{ name: "Return", channel: 8, range: high }, { name: "Kill", channel: 8, range: high }],
              expected: [{ channel: 8, users: ["Return", "Kill"] }] },
            { tag: "low and high split", assignments: [{ name: "Return", channel: 8, range: high }, { name: "Hold", channel: 8, range: low }],
              expected: [] },
            { tag: "halves meet without overlap", assignments: [{ name: "Return", channel: 8, range: upperHalf }, { name: "Hold", channel: 8, range: lowerHalf }],
              expected: [] },
            { tag: "stick under a switch", assignments: [{ name: "Throttle", channel: 3 }, { name: "Kill", channel: 3, range: high }],
              expected: [{ channel: 3, users: ["Throttle", "Kill"] }] },
            { tag: "only the overlapping pair", assignments: [{ name: "Return", channel: 8, range: high }, { name: "Hold", channel: 8, range: low },
                                                              { name: "Kill", channel: 8, range: high }],
              expected: [{ channel: 8, users: ["Return", "Kill"] }] },
            { tag: "unassigned ignored", assignments: [{ name: "Hold", channel: 0 }, { name: "Offboard", channel: 0 }],
              expected: [] },
            { tag: "mixed-sign three position", assignments: [{ name: "Payload", channel: 8, range: Rows.switchOnRange(0.75, -0.25) },
                                                              { name: "Hold", channel: 8, range: low }],
              expected: [] },
            { tag: "channel order", assignments: [{ name: "Return", channel: 8, range: high }, { name: "Kill", channel: 8, range: high },
                                                  { name: "Mode", channel: 5 }, { name: "Throttle", channel: 5 }],
              expected: [{ channel: 5, users: ["Mode", "Throttle"] }, { channel: 8, users: ["Return", "Kill"] }] }
        ]
    }

    function test_channelConflicts(data) {
        compare(Rows.channelConflicts(data.assignments), data.expected)
    }

    function test_externalModeIndex_data() {
        return [
            { tag: "slot built-in",     param: "COM_FLTMODE1",      value: 2,   expected: 0 },
            { tag: "slot external 1",   param: "COM_FLTMODE1",      value: 100, expected: 1 },
            { tag: "slot external 8",   param: "COM_FLTMODE6",      value: 107, expected: 8 },
            { tag: "boot built-in",     param: "COM_FLTMODE_BOOT",  value: 4,   expected: 0 },
            { tag: "boot external 1",   param: "COM_FLTMODE_BOOT",  value: 23,  expected: 1 },
            { tag: "boot external 8",   param: "COM_FLTMODE_BOOT",  value: 30,  expected: 8 }
        ]
    }

    function test_externalModeIndex(data) {
        compare(Rows.externalModeIndex(data.param, data.value), data.expected)
    }

    // Only external modes the vehicle has registered are offered, by their reported name. A slot already set to one
    // that is not registered stays listed so the combo box is not blank.
    function test_modeChoices_data() {
        var names = ["Survey Assist", "", "", "", "", "", "", ""]
        return [
            { tag: "registered named, unregistered hidden", current: 2, names: names,
              expected: [{ label: "Position", value: 2 }, { label: "Survey Assist", value: 100 }] },
            { tag: "current unregistered kept", current: 101, names: names,
              expected: [{ label: "Position", value: 2 }, { label: "Survey Assist", value: 100 },
                         { label: "External Mode 2 (not running)", value: 101 }] },
            { tag: "no names reported", current: 2, names: [],
              expected: [{ label: "Position", value: 2 }] }
        ]
    }

    function test_modeChoices(data) {
        compare(Rows.modeChoices("COM_FLTMODE1", ["Position", "External Mode 1", "External Mode 2"], [2, 100, 101],
                                 data.names, data.current),
                data.expected)
    }

    function test_modeName() {
        var names = ["Survey Assist", "", "", "", "", "", "", ""]
        var strings = ["Hold", "External 1", "External 2"]
        compare(Rows.modeName("COM_FLTMODE_BOOT", strings, [4, 23, 24], names, 23), "Survey Assist")
        compare(Rows.modeName("COM_FLTMODE_BOOT", strings, [4, 23, 24], names, 24), "External 2 (not running)")
        compare(Rows.modeName("COM_FLTMODE_BOOT", strings, [4, 23, 24], names, 99), "")
    }
}
