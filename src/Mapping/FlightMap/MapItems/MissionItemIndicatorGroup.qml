pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Groups mission item indicators which are too close to select individually.
Item {
    id: root

    required property FlightMap map
    required property QmlObjectListModel missionItems

    readonly property real groupingDistance: _mediumIndicatorRadius

    readonly property real _smallIndicatorRadius: _oddCeil((ScreenTools.defaultFontPixelHeight * ScreenTools.smallFontPointRatio) / 2)
    readonly property real _largeIndicatorRadius: _oddCeil(ScreenTools.defaultFontPixelHeight * 0.66)
    readonly property real _mediumIndicatorRadius: _oddCeil(((_smallIndicatorRadius * 2) + _largeIndicatorRadius) / 3)
    readonly property var _groupingState: {
        const state = []
        const count = missionItems ? missionItems.count : 0

        for (let i = 0; i < count; i++) {
            const item = missionItems.get(i)
            if (!item || !item.isSimpleItem || (!item.specifiesCoordinate && !item.isTakeoffItem)) {
                continue
            }

            const coordinate = _coordinateForItem(item)
            state.push({
                sequenceNumber: item.sequenceNumber,
                coordinateValid: coordinate && coordinate.isValid,
                latitude: coordinate && coordinate.isValid ? coordinate.latitude : NaN,
                longitude: coordinate && coordinate.isValid ? coordinate.longitude : NaN,
                current: item.isCurrentItem || item.hasCurrentChildItem
            })
        }

        return state
    }

    property var _groupsBySequenceNumber: ({})
    property var _selectionPanel

    signal itemSelected(int sequenceNumber)

    function groupForItem(item) {
        return item ? _groupsBySequenceNumber[item.sequenceNumber] || null : null
    }

    function showGroup(item, clickRect) {
        const group = groupForItem(item)
        if (!group || group.items.length < 2) {
            itemSelected(item.sequenceNumber)
            return
        }

        _closeSelectionPanel()
        const panel = selectionPanelComponent.createObject(mainWindow, {
            clickRect: clickRect,
            groupItems: group.items
        })
        if (!panel) {
            return
        }

        _selectionPanel = panel
        panel.open()
    }

    function _oddCeil(value) {
        const rounded = Math.ceil(value)
        return rounded + (rounded % 2 === 0 ? 1 : 0)
    }

    function _scheduleRegroup() {
        regroupTimer.restart()
    }

    function _coordinateForItem(item) {
        return item.isTakeoffItem && !item.specifiesCoordinate ? item.launchCoordinate : item.coordinate
    }

    function _closeSelectionPanel() {
        if (_selectionPanel) {
            _selectionPanel.close()
            _selectionPanel = null
        }
    }

    function _regroup() {
        _closeSelectionPanel()

        if (!map || !map.mapReady) {
            _groupsBySequenceNumber = {}
            return
        }

        const entries = []
        const count = missionItems ? missionItems.count : 0
        for (let i = 0; i < count; i++) {
            const item = missionItems.get(i)
            if (!item || !item.isSimpleItem || (!item.specifiesCoordinate && !item.isTakeoffItem)) {
                continue
            }

            const coordinate = _coordinateForItem(item)
            if (!coordinate || !coordinate.isValid) {
                continue
            }

            const point = map.fromCoordinate(coordinate, false /* clipToViewPort */)
            if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) {
                continue
            }

            entries.push({
                item: item,
                point: point,
                current: item.isCurrentItem || item.hasCurrentChildItem
            })
        }

        entries.sort((first, second) => {
            if (first.current !== second.current) {
                return first.current ? -1 : 1
            }
            return first.item.sequenceNumber - second.item.sequenceNumber
        })

        const representatives = MapMarkerGrouping.representatives(entries.map(entry => entry.point), groupingDistance)
        const groups = []
        const groupsByRepresentative = {}
        for (let i = 0; i < entries.length; i++) {
            const representative = representatives[i]
            if (!groupsByRepresentative[representative]) {
                groupsByRepresentative[representative] = { items: [], representative: entries[representative].item }
                groups.push(groupsByRepresentative[representative])
            }
            groupsByRepresentative[representative].items.push(entries[i].item)
        }

        const groupsBySequenceNumber = {}
        for (const group of groups) {
            group.items.sort((first, second) => first.sequenceNumber - second.sequenceNumber)
            for (const item of group.items) {
                groupsBySequenceNumber[item.sequenceNumber] = group
            }
        }
        _groupsBySequenceNumber = groupsBySequenceNumber
    }

    Timer {
        id: regroupTimer

        interval: 0
        repeat: false
        onTriggered: root._regroup()
    }

    Component {
        id: selectionPanelComponent

        MissionItemSelectionPanel {
            id: selectionPanel

            onItemSelected: (sequenceNumber) => root.itemSelected(sequenceNumber)

            onClosed: {
                if (root._selectionPanel === selectionPanel) {
                    root._selectionPanel = null
                }
                destroy()
            }
        }
    }

    Connections {
        target: root.map

        function onMapPanStop() { root._scheduleRegroup() }
        function onMapReadyChanged() { root._scheduleRegroup() }
        function onZoomLevelChanged() { root._scheduleRegroup() }
    }

    Connections {
        target: root.missionItems

        function onDataChanged() { root._scheduleRegroup() }
    }

    on_GroupingStateChanged: _scheduleRegroup()
    onGroupingDistanceChanged: _scheduleRegroup()
    onMapChanged: _scheduleRegroup()
    onMissionItemsChanged: _scheduleRegroup()

    Component.onCompleted: _scheduleRegroup()
    Component.onDestruction: _closeSelectionPanel()
}
