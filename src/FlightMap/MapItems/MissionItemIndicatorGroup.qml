pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.PlanView

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
        return item ? _groupsBySequenceNumber[item.sequenceNumber] : null
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
        Qt.callLater(_regroup)
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

        const cells = {}
        const groups = []
        for (const entry of entries) {
            const cellX = Math.floor(entry.point.x / groupingDistance)
            const cellY = Math.floor(entry.point.y / groupingDistance)
            let closestGroup = null
            let closestDistanceSquared = Infinity

            for (let x = cellX - 1; x <= cellX + 1; x++) {
                for (let y = cellY - 1; y <= cellY + 1; y++) {
                    const nearbyGroups = cells[`${x},${y}`] || []
                    for (const group of nearbyGroups) {
                        const deltaX = entry.point.x - group.point.x
                        const deltaY = entry.point.y - group.point.y
                        const distanceSquared = (deltaX * deltaX) + (deltaY * deltaY)
                        if (distanceSquared <= groupingDistance * groupingDistance
                                && (distanceSquared < closestDistanceSquared
                                    || (distanceSquared === closestDistanceSquared
                                        && group.representative.sequenceNumber < closestGroup.representative.sequenceNumber))) {
                            closestGroup = group
                            closestDistanceSquared = distanceSquared
                        }
                    }
                }
            }

            if (closestGroup) {
                closestGroup.items.push(entry.item)
                continue
            }

            const group = {
                items: [entry.item],
                point: entry.point,
                representative: entry.item
            }
            groups.push(group)

            const cellKey = `${cellX},${cellY}`
            if (!cells[cellKey]) {
                cells[cellKey] = []
            }
            cells[cellKey].push(group)
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

    Component {
        id: selectionPanelComponent

        DropPanel {
            id: selectionPanel

            modal: false

            required property var groupItems

            sourceComponent: Component {
                Item {
                    implicitWidth: itemListView.width
                    implicitHeight: itemListView.height

                    QGCListView {
                        id: itemListView

                        width: Math.min(Math.max(contentItem.childrenRect.width, _itemExtent), _maxWidth)
                        height: _itemExtent
                        orientation: ListView.Horizontal
                        model: selectionPanel.groupItems
                        cacheBuffer: width * 2
                        reuseItems: true
                        currentIndex: -1

                        readonly property real _itemExtent: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelHeight * 2.5)
                        readonly property real _maxWidth: selectionPanel.dropViewPort.width * 0.4

                        function _scrollBy(delta) {
                            const currentTarget = wheelScrollAnimation.running ? wheelScrollAnimation.to : contentX
                            const target = Math.max(0, Math.min(currentTarget - delta, Math.max(0, contentWidth - width)))
                            wheelScrollAnimation.stop()
                            wheelScrollAnimation.from = contentX
                            wheelScrollAnimation.to = target
                            wheelScrollAnimation.start()
                        }

                        delegate: Item {
                            id: itemDelegate

                            width: Math.max(itemListView._itemExtent, itemLabel.width + ScreenTools.defaultFontPixelWidth)
                            height: itemListView._itemExtent

                            required property var modelData

                            readonly property bool _usesAbbreviation: modelData.abbreviation.charAt(0) > 'A' && modelData.abbreviation.charAt(0) < 'z'
                            readonly property string _supplementaryLabel: !_usesAbbreviation ? "" : `${modelData.abbreviation} (${modelData.sequenceNumber})`

                            MissionItemIndexLabel {
                                id: itemLabel
                                anchors.centerIn: parent
                                checked: itemDelegate.modelData.isCurrentItem || itemDelegate.modelData.hasCurrentChildItem
                                label: itemDelegate.modelData.abbreviation
                                index: itemDelegate._usesAbbreviation ? -1 : itemDelegate.modelData.sequenceNumber
                                small: false
                                supplementaryLabel: itemDelegate._supplementaryLabel
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    selectionPanel.close()
                                    root.itemSelected(itemDelegate.modelData.sequenceNumber)
                                }
                            }
                        }

                        NumberAnimation {
                            id: wheelScrollAnimation

                            target: itemListView
                            property: "contentX"
                            duration: 120
                            easing.type: Easing.OutCubic
                        }

                        WheelHandler {
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            orientation: Qt.Vertical
                            target: null
                            onWheel: event => {
                                itemListView._scrollBy(event.pixelDelta.y || event.angleDelta.y)
                                event.accepted = true
                            }
                        }

                        WheelHandler {
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            orientation: Qt.Horizontal
                            target: null
                            onWheel: event => {
                                itemListView._scrollBy(event.pixelDelta.x || event.angleDelta.x)
                                event.accepted = true
                            }
                        }

                        onMovementStarted: wheelScrollAnimation.stop()
                    }
                }
            }

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
