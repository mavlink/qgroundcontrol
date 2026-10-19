import QtQuick
import QtQuick.Layouts
import QtTest

import "../../../../src/QmlControls"
import "../../../../src/QmlControls/QGCGroupBox.js" as GroupBox

/// Tests the pure tab selection and header button sizing logic used by QGCGroupBox.qml, and QGCGroupBoxTab layout.
TestCase {
    name: "QGCGroupBoxTest"

    // Stands in for the owning QGCGroupBox, which makes emptyTab the current tab
    QtObject {
        id: groupBoxStub
        property Item _currentTab: emptyTab
        function _validateCurrentIndex() { }
    }

    ColumnLayout {
        id: tabLayout
        width: 100
        height: 200
        spacing: 0

        QGCGroupBoxTab {
            id: emptyTab
            _groupBox: groupBoxStub
        }

        Item {
            id: sharedContent
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Tabs used only as selectors (e.g. ParameterEditor filters) must not take height from shared content
    function test_emptyTabDoesNotTakeSharedContentHeight() {
        verify(emptyTab.isCurrentTab)
        tryCompare(sharedContent, "height", 200, 1000)
        compare(sharedContent.y, 0)
    }

    QtObject {
        id: wideContentGroupBoxStub
        property Item _currentTab: wideContentTab
        function _validateCurrentIndex() { }
    }

    ColumnLayout {
        id: narrowLayout
        width: 100

        QGCGroupBoxTab {
            id: wideContentTab
            _groupBox: wideContentGroupBoxStub

            Item {
                Layout.minimumWidth: 300
                implicitHeight: 10
            }
        }
    }

    // Content wider than the group box must not widen the tab, which would push all of its rows past the border
    function test_tabStaysWithinContentWidth() {
        verify(wideContentTab.isCurrentTab)
        tryCompare(wideContentTab, "width", 100, 1000)
    }

    function test_validTabIndex_data() {
        return [
            { tag: "currentVisible", current: 1, flags: [true, true, true], expected: 1 },
            { tag: "currentHiddenFallsBackToFirstVisible", current: 0, flags: [false, true, true], expected: 1 },
            { tag: "noneSelectedPicksFirstVisible", current: -1, flags: [false, false, true], expected: 2 },
            { tag: "outOfRangePicksFirstVisible", current: 5, flags: [true, true], expected: 0 },
            { tag: "noVisibleTabs", current: 0, flags: [false, false], expected: -1 },
            { tag: "noTabs", current: 0, flags: [], expected: -1 },
        ]
    }

    function test_validTabIndex(data) {
        compare(GroupBox.validTabIndex(data.current, data.flags), data.expected)
    }

    function test_maxHeaderButtonWidth_data() {
        return [
            { tag: "allFitKeepNaturalWidth", widths: [20, 30], available: 100, spacing: 10, expected: 100 },
            { tag: "exactFit", widths: [45, 45], available: 100, spacing: 10, expected: 100 },
            { tag: "equalWideButtonsShareEvenly", widths: [80, 80], available: 100, spacing: 10, expected: 45 },
            { tag: "narrowButtonKeepsWidthWideShareRest", widths: [10, 100, 100], available: 100, spacing: 5, expected: 40 },
            { tag: "singleButtonCappedToAvailable", widths: [150], available: 100, spacing: 10, expected: 100 },
            { tag: "noRoomClampsToZero", widths: [50, 50], available: 5, spacing: 10, expected: 0 },
            { tag: "noButtons", widths: [], available: 100, spacing: 10, expected: 100 },
        ]
    }

    function test_maxHeaderButtonWidth(data) {
        compare(GroupBox.maxHeaderButtonWidth(data.widths, data.available, data.spacing), data.expected)
    }

    function test_sameItems_data() {
        const a = { name: "a" }
        const b = { name: "b" }
        return [
            { tag: "sameItemsSameOrder", first: [a, b], second: [a, b], expected: true },
            { tag: "bothEmpty", first: [], second: [], expected: true },
            { tag: "differentLength", first: [a], second: [a, b], expected: false },
            { tag: "sameItemsDifferentOrder", first: [a, b], second: [b, a], expected: false },
            { tag: "equalButDistinctObjects", first: [{ name: "a" }], second: [{ name: "a" }], expected: false },
        ]
    }

    function test_sameItems(data) {
        compare(GroupBox.sameItems(data.first, data.second), data.expected)
    }

    function test_topBorderSegments_data() {
        return [
            { tag: "noHeaderDrawsFullTopEdge", width: 100, radius: 10, gapStart: 10, gapEnd: 10,
              expected: [{ x: 10, width: 0 }, { x: 10, width: 80 }] },
            { tag: "gapBetweenCorners", width: 100, radius: 10, gapStart: 20, gapEnd: 60,
              expected: [{ x: 10, width: 10 }, { x: 60, width: 30 }] },
            { tag: "gapStartsInsideLeftCorner", width: 100, radius: 10, gapStart: 5, gapEnd: 60,
              expected: [{ x: 10, width: 0 }, { x: 60, width: 30 }] },
            { tag: "gapReachesRightCorner", width: 100, radius: 10, gapStart: 20, gapEnd: 95,
              expected: [{ x: 10, width: 10 }, { x: 90, width: 0 }] },
        ]
    }

    function test_topBorderSegments(data) {
        compare(GroupBox.topBorderSegments(data.width, data.radius, data.gapStart, data.gapEnd), data.expected)
    }

    function test_dividerLayout_data() {
        const contentLayout = { name: "content" }
        const currentTab = { name: "tab" }
        return [
            { tag: "noTabsUsesContentLayout", hasTabs: false, currentTab: null, contentLayout: contentLayout, expected: contentLayout },
            { tag: "tabsUseCurrentTab", hasTabs: true, currentTab: currentTab, contentLayout: contentLayout, expected: currentTab },
            { tag: "allTabsHiddenHasNoDividers", hasTabs: true, currentTab: null, contentLayout: contentLayout, expected: null },
        ]
    }

    function test_dividerLayout(data) {
        compare(GroupBox.dividerLayout(data.hasTabs, data.currentTab, data.contentLayout), data.expected)
    }

    function test_dividerVisibleAfter_data() {
        const shown = { visible: true, isRepeater: false }
        const hidden = { visible: false, isRepeater: false }
        const repeater = { visible: true, isRepeater: true }
        return [
            { tag: "visibleFollowedByVisible", index: 0, items: [shown, shown], expected: true },
            { tag: "lastItem", index: 1, items: [shown, shown], expected: false },
            { tag: "hiddenItem", index: 0, items: [hidden, shown], expected: false },
            { tag: "repeaterItem", index: 0, items: [repeater, shown], expected: false },
            { tag: "onlyHiddenBelow", index: 0, items: [shown, hidden, hidden], expected: false },
            { tag: "visibleBelowHidden", index: 0, items: [shown, hidden, shown], expected: true },
            { tag: "onlyRepeaterBelow", index: 0, items: [shown, repeater], expected: false },
            { tag: "visibleBelowRepeater", index: 0, items: [shown, repeater, shown], expected: true },
            { tag: "missingItem", index: 0, items: [null, shown], expected: false },
        ]
    }

    function test_dividerVisibleAfter(data) {
        compare(GroupBox.dividerVisibleAfter(data.index, data.items), data.expected)
    }
}
