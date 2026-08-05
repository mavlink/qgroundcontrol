import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.PlanView

/// Unified plan tree view showing Mission Items, GeoFence, and Rally Points
/// as collapsible sections using a real TreeView with type-discriminating delegates.
TreeView {
    id: root
    model: _missionController.visualItemsTree
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    reuseItems: false
    pointerNavigationEnabled: false
    selectionBehavior: TableView.SelectionDisabled
    rowSpacing: 2

    required property var editorMap
    required property var planMasterController

    signal editingLayerChangeRequested(int layer)

    readonly property bool _createNewPlanMode: planMasterController.showCreateFromTemplate

    on_CreateNewPlanModeChanged: {
        if (_createNewPlanMode) {
            var planFileRow = _rowFor(_missionController.planFileGroupIndex)
            if (!root.isExpanded(planFileRow)) {
                root.expand(planFileRow)
            }
            root.contentY = 0
        }
    }

    property var _missionController: planMasterController.missionController
    property var _geoFenceController: planMasterController.geoFenceController
    property var _rallyPointController: planMasterController.rallyPointController

    // Helper: convert a persistent model index to the current visual row
    function _rowFor(modelIndex) { return root.rowAtIndex(modelIndex) }

    // QGCFlickableScrollIndicator expects parent to have indicatorColor (provided by QGCFlickable/QGCListView)
    property color indicatorColor: qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    QGCFlickableScrollIndicator { parent: root; orientation: QGCFlickableScrollIndicator.Horizontal }
    QGCFlickableScrollIndicator { parent: root; orientation: QGCFlickableScrollIndicator.Vertical }

    property int _lastMissionItemCount: 0

    Connections {
        target: root._missionController.visualItems
        function onCountChanged() {
            var newCount = _missionController.visualItems ? _missionController.visualItems.count : 0
            if (newCount > root._lastMissionItemCount) {
                // First waypoint added — collapse Plan Info and Defaults
                if (root._lastMissionItemCount <= 1 && newCount > 1) {
                    var planFileRow = _rowFor(_missionController.planFileGroupIndex)
                    if (root.isExpanded(planFileRow)) {
                        root.collapse(planFileRow)
                    }
                    var defaultsRow = _rowFor(_missionController.defaultsGroupIndex)
                    if (root.isExpanded(defaultsRow)) {
                        root.collapse(defaultsRow)
                    }
                }
                // Expand mission group (making Mission the active layer) and scroll to the new item
                root.selectLayer("missionGroup")
                // Scroll happens when the editor signals editorExpandedAndLoaded
            }
            root._lastMissionItemCount = newCount
        }
    }

    Connections {
        target: root._missionController
        function onVisualItemsReset() {
            root.collapseRecursively()
            if (_missionController.containsItems) {
                // Non-empty plan: expand mission group
                root.expand(_rowFor(_missionController.missionGroupIndex))
            } else {
                // Empty plan: expand Plan Info and Defaults, scroll to top
                root.expand(_rowFor(_missionController.planFileGroupIndex))
                root.expand(_rowFor(_missionController.defaultsGroupIndex))
                root.contentY = 0
            }
            root._lastMissionItemCount = _missionController.visualItems ? _missionController.visualItems.count : 0
            root.editingLayerChangeRequested(PlanEditLayers.layerMission)
        }
        function onPlanViewStateChanged() {
            // Current item changed — bring it on-screen if completely off-screen.
            // Fine-tuned scroll happens later via editorExpandedAndLoaded.
            var item = _missionController.currentPlanViewItem
            if (item) {
                var modelIndex = _missionController.visualItemsTree.indexForObject(item)
                var row = root.rowAtIndex(modelIndex)
                if (row >= 0) {
                    root.forceLayout()
                    root.positionViewAtRow(row, TableView.Visible)
                }
            }
        }
    }

    // The three groups which correspond to map editing layers. These are exclusive:
    // expanding one collapses the others and makes it the active editing layer.

    function _layerGroupRow(nodeType) {
        switch (nodeType) {
        case "missionGroup":    return _rowFor(_missionController.missionGroupIndex)
        case "fenceGroup":      return _rowFor(_missionController.fenceGroupIndex)
        case "rallyGroup":      return _rowFor(_missionController.rallyGroupIndex)
        }
        return -1
    }

    function _collapseOtherLayerGroups(nodeType) {
        for (var layerInfo of PlanEditLayers.layerInfos) {
            if (layerInfo.nodeType !== nodeType) {
                var row = _layerGroupRow(layerInfo.nodeType)
                if (row >= 0 && root.isExpanded(row)) {
                    root.collapse(row)
                }
            }
        }
    }

    // Public API: make a layer group the active editing layer and expand it,
    // collapsing the other layer groups. Called by the layer tool buttons and group headers.
    function selectLayer(nodeType) {
        var layerInfo = PlanEditLayers.infoForNodeType(nodeType)
        if (!layerInfo) {
            return
        }
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        editingLayerChangeRequested(layerInfo.layer)
        _collapseOtherLayerGroups(nodeType)
        var targetRow = _layerGroupRow(nodeType)
        if (targetRow >= 0) {
            if (!root.isExpanded(targetRow)) {
                root.expand(targetRow)
            }
            root.forceLayout()
            root.positionViewAtRow(targetRow, TableView.AlignTop)
        }
    }

    // Toggle expand/collapse for a non-layer group header (Plan Info, Defaults, Transform).
    // Caller is responsible for calling allowViewSwitch() before invoking this.
    function _toggleGroup(row) {
        if (root.isExpanded(row)) {
            root.collapse(row)
        } else {
            root.expand(row)
        }
        root.forceLayout()
    }

    // Subtitle text shown on group headers, varies by node type
    function _groupSubtitle(nodeType) {
        switch (nodeType) {
        case "planFileGroup":   return planMasterController.currentPlanFileName === "" ? qsTr("<Untitled>") : planMasterController.currentPlanFileName
        case "missionGroup":    return _missionController.visualItems ? qsTr("%1 items").arg(_missionController.visualItems.count - 1) : ""
        case "rallyGroup":      return _rallyPointController.points ? qsTr("%1 points").arg(_rallyPointController.points.count) : ""
        default:                return ""
        }
    }

    // Icon shown on group headers, matches the layer switcher icons
    function _groupIcon(nodeType) {
        return PlanEditLayers.infoForNodeType(nodeType)?.icon ?? ""
    }

    // Coalesces multiple delegate height changes into a single forceLayout() call
    Timer {
        id: layoutTimer
        interval: 0
        running: false
        repeat: false
        onTriggered: root.forceLayout()
    }

    // Called by MissionItemEditor delegates when their editor height has settled.
    function _scrollToMissionItem(delegateItem) {
        root.forceLayout()
        var bottomY = delegateItem.mapToItem(root.contentItem, 0, delegateItem.height).y
        var neededContentY = bottomY - root.height
        if (neededContentY > root.contentY) {
            root.contentY = neededContentY
        }
    }

    delegate: Item {
        id: delegateRoot
        implicitWidth: root.width
        implicitHeight: (loader.item ? loader.item.height : 1) + (separatorLine.visible ? separatorLine.height + root.rowSpacing : 0)
        enabled: !root._createNewPlanMode || _enabledInCreateMode
        opacity: enabled ? 1 : root.editorMap._nonInteractiveOpacity
        width: root.width

        required property TreeView treeView
        required property bool isTreeNode
        required property bool expanded
        required property bool hasChildren
        required property int depth
        required property int row
        required property var model

        readonly property var nodeObject: model.object
        readonly property string nodeType: model.nodeType
        readonly property bool separator: model.separator ?? false

        // In create-new-plan mode, only the Plan Info and Defaults groups and their children are enabled
        readonly property bool _enabledInCreateMode: nodeType === "planFileGroup" || nodeType === "planFileInfo"
                                                     || nodeType === "defaultsGroup" || nodeType === "defaultsInfo"

        onImplicitHeightChanged: layoutTimer.restart()

        readonly property string _qrcBase: "qrc:/qml/QGroundControl/PlanView/"

        // We use setSource() instead of sourceComponent so that required properties
        // (e.g. missionItem) are injected before internal bindings activate,
        // preventing "Cannot read property of null" warnings.
        Loader {
            id: loader
            width: parent.width

            Component.onCompleted: {
                switch (delegateRoot.nodeType) {
                case "planFileGroup":
                case "defaultsGroup":
                case "missionGroup":
                case "fenceGroup":
                case "rallyGroup":
                case "transformGroup":
                    sourceComponent = groupHeaderComponent
                    break
                case "planFileInfo":
                    setSource(delegateRoot._qrcBase + "PlanInfoEditor.qml", {
                        objectName:             "planTree_planFileInfo",
                        width:                  Qt.binding(() => delegateRoot.width),
                        planMasterController:   root.planMasterController,
                        missionController:      root._missionController,
                        editorMap:              root.editorMap
                    })
                    break
                case "defaultsInfo":
                    setSource(delegateRoot._qrcBase + "MissionDefaultsEditor.qml", {
                        width:                  Qt.binding(() => delegateRoot.width),
                        missionController:      root._missionController,
                        planMasterController:   root.planMasterController
                    })
                    break
                case "missionItem":
                    if (delegateRoot.nodeObject) {
                        setSource(delegateRoot._qrcBase + "MissionItemEditor.qml", {
                            objectName:     "planTree_missionItemEditor",
                            width:          Qt.binding(() => delegateRoot.width),
                            map:            root.editorMap,
                            missionItem:    delegateRoot.nodeObject
                        })
                    }
                    break
                case "fenceEditor":
                    if (delegateRoot.nodeObject) {
                        setSource(delegateRoot._qrcBase + "GeoFenceEditor.qml", {
                            objectName:             "planTree_fenceEditor",
                            width:                  Qt.binding(() => delegateRoot.width),
                            myGeoFenceController:   root._geoFenceController,
                            flightMap:              root.editorMap
                        })
                    }
                    break
                case "rallyHeader":
                    if (delegateRoot.nodeObject) {
                        setSource(delegateRoot._qrcBase + "RallyPointEditorHeader.qml", {
                            objectName: "planTree_rallyHeader",
                            width:      Qt.binding(() => delegateRoot.width),
                            controller: root._rallyPointController
                        })
                    }
                    break
                case "rallyItem":
                    if (delegateRoot.nodeObject) {
                        setSource(delegateRoot._qrcBase + "RallyPointItemEditor.qml", {
                            width:      Qt.binding(() => delegateRoot.width),
                            rallyPoint: delegateRoot.nodeObject,
                            controller: root._rallyPointController
                        })
                    }
                    break
                case "transformEditor":
                    setSource(delegateRoot._qrcBase + "TransformEditor.qml", {
                        width:              Qt.binding(() => delegateRoot.width),
                        missionController:  root._missionController
                    })
                    break
                }
            }

            onLoaded: {
                if (delegateRoot.nodeType === "missionItem" && item) {
                    item.clicked.connect(function() {
                        root._missionController.setCurrentPlanViewSeqNum(delegateRoot.nodeObject.sequenceNumber, false)
                    })
                    item.remove.connect(function() {
                        var viIndex = root._missionController.visualItemIndexForObject(delegateRoot.nodeObject)
                        if (viIndex > 0) {
                            root._missionController.removeVisualItem(viIndex)
                        }
                    })
                    item.selectNextNotReadyItem.connect(function() {
                        for (var i = 0; i < root._missionController.visualItems.count; i++) {
                            var vmi = root._missionController.visualItems.get(i)
                            if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                                root._missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                                break
                            }
                        }
                    })
                    item.editorExpandedAndLoaded.connect(function() {
                        root._scrollToMissionItem(delegateRoot)
                    })
                }
            }
        }

        Rectangle {
            id: separatorLine
            anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
            anchors.topMargin: root.rowSpacing
            anchors.top: loader.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: qgcPal.groupBorder
            visible: delegateRoot.separator
        }

        // ── Group header (Mission Items / GeoFence / Rally Points) ──
        Component {
            id: groupHeaderComponent

            Rectangle {
                objectName: "planTree_" + delegateRoot.nodeType + "Header"
                width:  delegateRoot.width
                height: ScreenTools.implicitComboBoxHeight + ScreenTools.defaultFontPixelWidth
                color:  qgcPal.windowShade

                RowLayout {
                    id: groupHeaderRow
                    spacing: ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5

                    QGCColoredImage {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.75
                        Layout.preferredHeight: Layout.preferredWidth
                        source: "/InstrumentValueIcons/cheveron-right.svg"
                        color: qgcPal.text
                        rotation: delegateRoot.expanded ? 90 : 0
                    }

                    QGCLabel {
                        Layout.alignment: Qt.AlignBaseline
                        text: delegateRoot.nodeObject ? delegateRoot.nodeObject.objectName : ""
                        font.bold: true
                    }

                    QGCColoredImage {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.preferredWidth: ScreenTools.defaultFontPixelHeight * 0.75
                        Layout.preferredHeight: Layout.preferredWidth
                        source: root._groupIcon(delegateRoot.nodeType)
                        color: qgcPal.text
                        visible: source.toString() !== ""
                    }

                    QGCLabel {
                        Layout.alignment: Qt.AlignBaseline
                        Layout.fillWidth: true
                        text: root._groupSubtitle(delegateRoot.nodeType)
                        elide: Text.ElideRight
                        font.pointSize: ScreenTools.smallFontPointSize
                        color: qgcPal.colorGrey
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (PlanEditLayers.infoForNodeType(delegateRoot.nodeType)) {
                            // Layer groups cannot be collapsed by clicking their header. The active
                            // editing layer always has its group expanded. selectLayer() handles
                            // the allowViewSwitch() guard.
                            root.selectLayer(delegateRoot.nodeType)
                        } else {
                            if (!mainWindow.allowViewSwitch()) {
                                return
                            }
                            root._toggleGroup(delegateRoot.row)
                        }
                    }
                }
            }
        }

    }
}
