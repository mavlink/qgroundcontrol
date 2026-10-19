.pragma library

/// Returns the tab index to show: currentIndex while that tab is visible, otherwise the first visible tab, or -1
/// when no tab is visible.
function validTabIndex(currentIndex, tabVisibleFlags) {
    if (currentIndex >= 0 && currentIndex < tabVisibleFlags.length && tabVisibleFlags[currentIndex]) {
        return currentIndex
    }
    return tabVisibleFlags.indexOf(true)
}

/// Returns the width cap for header buttons so they fit on one row of availableWidth. Buttons narrower than their fair
/// share keep their natural width and the remaining width is split evenly between the wider buttons.
function maxHeaderButtonWidth(buttonImplicitWidths, availableWidth, spacing) {
    const count = buttonImplicitWidths.length
    let remainingWidth = availableWidth - spacing * Math.max(count - 1, 0)
    const sortedWidths = buttonImplicitWidths.slice().sort((a, b) => a - b)
    for (let i = 0; i < count; i++) {
        const share = remainingWidth / (count - i)
        if (sortedWidths[i] > share) {
            return Math.max(share, 0)
        }
        remainingWidth -= sortedWidths[i]
    }
    return availableWidth
}

/// Returns true when both lists hold the same objects in the same order.
function sameItems(first, second) {
    return first.length === second.length && first.every((item, index) => item === second[index])
}

/// Returns the left and right segments of the straight top border edge, which runs between the rounded corners and
/// leaves a gap from gapStart to gapEnd for the header buttons.
function topBorderSegments(borderWidth, radius, gapStart, gapEnd) {
    const leftEnd = Math.max(Math.min(gapStart, borderWidth - radius), radius)
    const rightStart = Math.min(Math.max(gapEnd, radius), borderWidth - radius)
    return [
        { x: radius, width: leftEnd - radius },
        { x: rightStart, width: borderWidth - radius - rightStart },
    ]
}

/// Returns the layout whose children are separated by dividers: the current tab when there are tabs, which is null
/// when no tab is visible, otherwise the group box content layout.
function dividerLayout(hasTabs, currentTab, contentLayout) {
    return hasTabs ? currentTab : contentLayout
}

/// Returns true when a divider is drawn below items[index]. items are ordered top to bottom as { visible, isRepeater }.
/// Repeaters draw nothing so they never get a divider, and a divider is only drawn when a visible item follows.
function dividerVisibleAfter(index, items) {
    const item = items[index]
    if (!item || !item.visible || item.isRepeater) {
        return false
    }
    for (let i = index + 1; i < items.length; i++) {
        if (items[i] && items[i].visible && !items[i].isRepeater) {
            return true
        }
    }
    return false
}
