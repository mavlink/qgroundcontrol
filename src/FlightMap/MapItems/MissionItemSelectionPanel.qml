pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.PlanView

/// Picker for mission item markers drawn too close together to click individually
DropPanel {
    id: root

    modal: false

    required property var groupItems

    signal itemSelected(int sequenceNumber)

    sourceComponent: Component {
        Item {
            implicitWidth: itemListView.width
            implicitHeight: itemListView.height

            QGCListView {
                id: itemListView

                objectName: "missionItemSelectionList"
                width: Math.min(Math.max(contentItem.childrenRect.width, _itemExtent), _maxWidth)
                height: _itemExtent
                orientation: ListView.Horizontal
                model: root.groupItems
                cacheBuffer: width * 2
                reuseItems: true
                currentIndex: -1

                readonly property real _itemExtent: Math.max(ScreenTools.minTouchPixels, ScreenTools.defaultFontPixelHeight * 2.5)
                readonly property real _maxWidth: root.dropViewPort.width * 0.4

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

                    objectName: "missionItemSelection_" + modelData.sequenceNumber
                    width: Math.max(itemListView._itemExtent, itemLabel.width + ScreenTools.defaultFontPixelWidth)
                    height: itemListView._itemExtent

                    required property var modelData

                    readonly property bool _usesAbbreviation: modelData.abbreviation !== ""
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
                            root.close()
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
}
