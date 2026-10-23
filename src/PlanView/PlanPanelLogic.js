.pragma library

const TabPlan = "plan"
const TabMission = "mission"
const TabFence = "fence"
const TabRally = "rally"

const tabIds = [TabPlan, TabMission, TabFence, TabRally]

// Below this panel height (in default font heights) the list and the inspector no longer fit stacked
const splitMinHeightInFontHeights = 32

function layoutMode(panelHeight, fontPixelHeight) {
    if (!(fontPixelHeight > 0) || !(panelHeight >= 0)) {
        return "split"
    }
    return panelHeight >= splitMinHeightInFontHeights * fontPixelHeight ? "split" : "drillIn"
}

// Below this height (in default font heights) the vertical text tabs no longer fit and the tabs fall back to icons
const textTabsMinHeightInFontHeights = 28

function tabStyle(availableHeight, fontPixelHeight) {
    if (!(fontPixelHeight > 0) || !(availableHeight >= 0)) {
        return "icon"
    }
    return availableHeight >= textTabsMinHeightInFontHeights * fontPixelHeight ? "text" : "icon"
}

function defaultTab(containsItems) {
    return containsItems ? TabMission : TabPlan
}

function tabAfterItemAdded(currentTab) {
    return currentTab === TabPlan ? TabMission : currentTab
}

function incompleteItemCount(readyStates, readyValue) {
    let count = 0
    for (const state of readyStates) {
        if (state !== readyValue) {
            count++
        }
    }
    return count
}

// Entries for the tab pills, in tab order. warn shows a warning dot on the pill.
function tabModel(summary) {
    return [
        { id: TabPlan,    warn: false },
        { id: TabMission, warn: summary.incompleteCount > 0 },
        { id: TabFence,   warn: false },
        { id: TabRally,   warn: false },
    ]
}

// Index of the item reached by stepping delta items from current, or -1 when that leaves the list
function steppedIndex(current, delta, itemCount) {
    const target = current + delta
    if (current < 0 || current >= itemCount || target < 0 || target >= itemCount) {
        return -1
    }
    return target
}

// Keeps both the list and the inspector at least their minimum height when the divider is dragged
function clampedListHeight(requested, totalHeight, dividerHeight, minList, minInspector) {
    const maxList = Math.max(minList, totalHeight - dividerHeight - minInspector)
    return Math.min(Math.max(requested, minList), maxList)
}

function formatDuration(totalSeconds) {
    const seconds = totalSeconds > 0 ? Math.floor(totalSeconds) : 0
    const pad = value => (value < 10 ? "0" : "") + value
    return pad(Math.floor(seconds / 3600)) + ":" + pad(Math.floor((seconds % 3600) / 60)) + ":" + pad(seconds % 60)
}
