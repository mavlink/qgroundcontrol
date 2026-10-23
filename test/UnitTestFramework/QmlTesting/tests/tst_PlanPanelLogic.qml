import QtQuick
import QtTest

import "../../../../src/PlanView/PlanPanelLogic.js" as Logic

/// Tests the pure tab, layout and stepping rules behind PlanPanel.qml.
TestCase {
    name: "PlanPanelLogicTest"

    function test_layoutMode_data() {
        const font = 10
        const limit = Logic.splitMinHeightInFontHeights * font
        return [
            { tag: "tallPanelSplits", height: limit + 1, font: font, expected: "split" },
            { tag: "exactLimitSplits", height: limit, font: font, expected: "split" },
            { tag: "justBelowLimitDrillsIn", height: limit - 1, font: font, expected: "drillIn" },
            { tag: "shortPanelDrillsIn", height: 5 * font, font: font, expected: "drillIn" },
            { tag: "thresholdScalesWithFont", height: limit, font: font * 2, expected: "drillIn" },
            { tag: "zeroFontKeepsSplit", height: 100, font: 0, expected: "split" },
            { tag: "unsetFontKeepsSplit", height: 100, font: undefined, expected: "split" },
            { tag: "negativeHeightKeepsSplit", height: -1, font: font, expected: "split" },
        ]
    }

    function test_layoutMode(data) {
        compare(Logic.layoutMode(data.height, data.font), data.expected)
    }

    function test_tabStyle_data() {
        const font = 10
        const limit = Logic.textTabsMinHeightInFontHeights * font
        return [
            { tag: "tallPanelUsesText", height: limit + 1, font: font, expected: "text" },
            { tag: "exactLimitUsesText", height: limit, font: font, expected: "text" },
            { tag: "justBelowLimitUsesIcons", height: limit - 1, font: font, expected: "icon" },
            { tag: "thresholdScalesWithFont", height: limit, font: font * 2, expected: "icon" },
            { tag: "zeroFontUsesIcons", height: 1000, font: 0, expected: "icon" },
            { tag: "unsetFontUsesIcons", height: 1000, font: undefined, expected: "icon" },
            { tag: "negativeHeightUsesIcons", height: -1, font: font, expected: "icon" },
        ]
    }

    function test_tabStyle(data) {
        compare(Logic.tabStyle(data.height, data.font), data.expected)
    }

    function test_defaultTab_data() {
        return [
            { tag: "emptyPlanStartsOnPlan", containsItems: false, expected: Logic.TabPlan },
            { tag: "planWithItemsStartsOnMission", containsItems: true, expected: Logic.TabMission },
        ]
    }

    function test_defaultTab(data) {
        compare(Logic.defaultTab(data.containsItems), data.expected)
    }

    function test_tabAfterItemAdded_data() {
        return [
            { tag: "planLeavesForMission", current: Logic.TabPlan, expected: Logic.TabMission },
            { tag: "missionStays", current: Logic.TabMission, expected: Logic.TabMission },
            { tag: "fenceStays", current: Logic.TabFence, expected: Logic.TabFence },
            { tag: "rallyStays", current: Logic.TabRally, expected: Logic.TabRally },
        ]
    }

    function test_tabAfterItemAdded(data) {
        compare(Logic.tabAfterItemAdded(data.current), data.expected)
    }

    function test_incompleteItemCount_data() {
        return [
            { tag: "allReady", states: [0, 0, 0], expected: 0 },
            { tag: "mixed", states: [0, 1, 2, 0], expected: 2 },
            { tag: "none", states: [], expected: 0 },
        ]
    }

    function test_incompleteItemCount(data) {
        compare(Logic.incompleteItemCount(data.states, 0), data.expected)
    }

    function test_tabModel_orderAndIds() {
        const model = Logic.tabModel({ incompleteCount: 0 })
        compare(model.map(function (entry) { return entry.id }), Logic.tabIds)
        compare(Logic.tabIds[0], Logic.TabPlan)
    }

    function test_tabModel_data() {
        return [
            { tag: "noWarnings", incompleteCount: 0, warns: [false, false, false, false] },
            { tag: "incompleteItemsWarnOnMission", incompleteCount: 3, warns: [false, true, false, false] },
        ]
    }

    function test_tabModel(data) {
        const model = Logic.tabModel({ incompleteCount: data.incompleteCount })
        compare(model.map(function (entry) { return entry.warn }), data.warns)
    }

    function test_steppedIndex_data() {
        return [
            { tag: "next", current: 2, delta: 1, count: 5, expected: 3 },
            { tag: "previous", current: 2, delta: -1, count: 5, expected: 1 },
            { tag: "previousFromFirstStops", current: 0, delta: -1, count: 5, expected: -1 },
            { tag: "nextFromLastStops", current: 4, delta: 1, count: 5, expected: -1 },
            { tag: "singleItemHasNoNeighbors", current: 0, delta: 1, count: 1, expected: -1 },
            { tag: "emptyList", current: 0, delta: 1, count: 0, expected: -1 },
            { tag: "noCurrentItem", current: -1, delta: 1, count: 5, expected: -1 },
            { tag: "currentPastEnd", current: 7, delta: -1, count: 5, expected: -1 },
        ]
    }

    function test_steppedIndex(data) {
        compare(Logic.steppedIndex(data.current, data.delta, data.count), data.expected)
    }

    function test_clampedListHeight_data() {
        // total 400, divider 10, list min 40, inspector min 100 -> list range 40..290
        return [
            { tag: "insideRangeKept", requested: 150, expected: 150 },
            { tag: "belowMinimumRaised", requested: 5, expected: 40 },
            { tag: "negativeRaisedToMinimum", requested: -20, expected: 40 },
            { tag: "aboveMaximumLeavesInspectorMinimum", requested: 380, expected: 290 },
            { tag: "exactMinimumKept", requested: 40, expected: 40 },
            { tag: "exactMaximumKept", requested: 290, expected: 290 },
        ]
    }

    function test_clampedListHeight(data) {
        compare(Logic.clampedListHeight(data.requested, 400, 10, 40, 100), data.expected)
    }

    function test_clampedListHeight_tooShortForBothMinimums() {
        // The list minimum wins so the list never collapses
        compare(Logic.clampedListHeight(500, 100, 10, 40, 100), 40)
    }

    function test_formatDuration_data() {
        return [
            { tag: "zero", seconds: 0, expected: "00:00:00" },
            { tag: "secondsOnly", seconds: 9, expected: "00:00:09" },
            { tag: "minutesAndSeconds", seconds: 494, expected: "00:08:14" },
            { tag: "hours", seconds: 3725, expected: "01:02:05" },
            { tag: "fractionTruncates", seconds: 59.9, expected: "00:00:59" },
            { tag: "negativeClampsToZero", seconds: -5, expected: "00:00:00" },
            { tag: "nanClampsToZero", seconds: NaN, expected: "00:00:00" },
            { tag: "overADay", seconds: 90000, expected: "25:00:00" },
        ]
    }

    function test_formatDuration(data) {
        compare(Logic.formatDuration(data.seconds), data.expected)
    }
}
