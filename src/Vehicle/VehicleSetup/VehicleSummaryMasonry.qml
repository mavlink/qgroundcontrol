import QtQuick

/// Lays the delegates out in as many columns as fit, left to right, placing each one in the column that is currently
/// shortest so a row never leaves a gap under a shorter item. Delegates that are not visible are skipped.
Item {
    id: control

    property alias model:       repeater.model
    property alias delegate:    repeater.delegate

    property real minColumnWidth:   100
    property real spacing:          0

    readonly property int columnCount: Math.max(1, Math.floor((width + spacing) / (minColumnWidth + spacing)))

    onWidthChanged:         _scheduleLayout()
    onColumnCountChanged:   _scheduleLayout()
    onSpacingChanged:       _scheduleLayout()

    function _scheduleLayout() {
        layoutTimer.restart()
    }

    function _layout() {
        const columnWidth = (width - spacing * (columnCount - 1)) / columnCount
        const columnBottoms = new Array(columnCount).fill(0)
        for (let i = 0; i < repeater.count; i++) {
            const item = repeater.itemAt(i)
            if (!item || !item.visible) {
                continue
            }
            let column = 0
            for (let c = 1; c < columnCount; c++) {
                if (columnBottoms[c] < columnBottoms[column]) {
                    column = c
                }
            }
            item.width = columnWidth
            item.x = column * (columnWidth + spacing)
            item.y = columnBottoms[column]
            columnBottoms[column] += item.height + spacing
        }
        implicitHeight = Math.max(0, Math.max(...columnBottoms) - spacing)
    }

    Timer {
        id:             layoutTimer
        interval:       0
        onTriggered:    control._layout()
    }

    Repeater {
        id: repeater

        onItemAdded: (index, item) => {
            item.heightChanged.connect(control._scheduleLayout)
            item.visibleChanged.connect(control._scheduleLayout)
            control._scheduleLayout()
        }
        onItemRemoved: (index, item) => {
            item.heightChanged.disconnect(control._scheduleLayout)
            item.visibleChanged.disconnect(control._scheduleLayout)
            control._scheduleLayout()
        }
    }

    // The items are destroyed after this item, and must not call back into it
    Component.onDestruction: {
        for (let i = 0; i < repeater.count; i++) {
            const item = repeater.itemAt(i)
            if (item) {
                item.heightChanged.disconnect(control._scheduleLayout)
                item.visibleChanged.disconnect(control._scheduleLayout)
            }
        }
    }
}
