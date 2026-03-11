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

    readonly property int _layerMission: 1
    readonly property int _layerFence:   2
    readonly property int _layerRally:   3

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

    Connections {
        target: root._missionController
        function onVisualItemsChanged() {
            // Mission group always expanded after rebuild (clear / load)
            root.collapseRecursively()
            root.expand(_rowFor(_missionController.missionGroupIndex))
            root.editingLayerChangeRequested(root._layerMission)
        }
    }

    // Public API: select a layer and expand its group. Called by the layer tool buttons.
    function selectLayer(nodeType) {
        let targetRow = -1
        switch (nodeType) {
        case "missionGroup":
            targetRow = _rowFor(_missionController.missionGroupIndex)
            editingLayerChangeRequested(_layerMission)
            break
        case "fenceGroup":
            targetRow = _rowFor(_missionController.fenceGroupIndex)
            editingLayerChangeRequested(_layerFence)
            break
        case "rallyGroup":
            targetRow = _rowFor(_missionController.rallyGroupIndex)
            editingLayerChangeRequested(_layerRally)
            break
        }

        if (targetRow >= 0) {
            if (!root.isExpanded(targetRow))
                root.expand(targetRow)
            root.forceLayout()
            root.positionViewAtRow(targetRow, TableView.AlignTop)
        }
    }

    // Toggle expand/collapse for a group header. Does not affect the editing layer.
    function _toggleGroup(row) {
        if (root.isExpanded(row))
            root.collapse(row)
        else
            root.expand(row)
        root.forceLayout()
    }

    // Subtitle text shown on group headers, varies by node type
    function _groupSubtitle(nodeType) {
        switch (nodeType) {
        case "planFileGroup":   return planMasterController.currentPlanFileName === "" ? qsTr("<Untitled>") : planMasterController.currentPlanFileName
        case "missionGroup":    return _missionController.visualItems ? (_missionController.visualItems.count - 1) + qsTr(" items") : ""
        case "rallyGroup":      return _rallyPointController.points ? _rallyPointController.points.count + qsTr(" points") : ""
        default:                return ""
        }
    }

    // Coalesces multiple delegate height changes into a single forceLayout() call
    Timer {
        id: layoutTimer
        interval: 0
        running: false
        repeat: false
        onTriggered: root.forceLayout()
    }

    delegate: Item {
        id: delegateRoot
        implicitWidth: root.width
        implicitHeight: (loader.item ? loader.item.height : 1) + (separatorLine.visible ? separatorLine.height + root.rowSpacing : 0)
        width: root.width
        height: implicitHeight

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


        onImplicitHeightChanged: layoutTimer.restart()

        Loader {
            id: loader
            width: parent.width
            sourceComponent: {
                // Guard: non-group delegates need a valid object. During model
                // row removal the role data goes null before the delegate is
                // destroyed, which would cause "Cannot read property of null"
                // warnings in every downstream editor binding.
                switch (delegateRoot.nodeType) {
                case "planFileGroup":   return groupHeaderComponent
                case "defaultsGroup":   return groupHeaderComponent
                case "missionGroup":    return groupHeaderComponent
                case "fenceGroup":      return groupHeaderComponent
                case "rallyGroup":      return groupHeaderComponent
                case "transformGroup":  return groupHeaderComponent
                case "planFileInfo":    return planFileInfoComponent
                case "defaultsInfo":    return defaultsEditorComponent
                case "missionItem":     return delegateRoot.nodeObject ? missionItemComponent  : null
                case "fenceEditor":     return delegateRoot.nodeObject ? fenceEditorComponent  : null
                case "rallyHeader":     return delegateRoot.nodeObject ? rallyHeaderComponent  : null
                case "rallyItem":       return delegateRoot.nodeObject ? rallyItemComponent    : null
                case "transformEditor": return transformEditorComponent
                default:                return null
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
                    onClicked: root._toggleGroup(delegateRoot.row)
                }
            }
        }

        // ── Plan info delegate ──
        Component {
            id: planFileInfoComponent

            PlanInfoEditor {
                width: delegateRoot.width
                planMasterController: root.planMasterController
                missionController: root._missionController
                editorMap: root.editorMap
            }
        }

        // ── Defaults editor delegate ──
        Component {
            id: defaultsEditorComponent

            MissionDefaultsEditor {
                width: delegateRoot.width
                missionController: root._missionController
                planMasterController: root.planMasterController
            }
        }

        // ── Mission item delegate ──
        Component {
            id: missionItemComponent

            MissionItemEditor {
                width: delegateRoot.width
                map: root.editorMap
                masterController: root.planMasterController
                missionItem: delegateRoot.nodeObject
                readOnly: false

                onClicked:  root._missionController.setCurrentPlanViewSeqNum(delegateRoot.nodeObject.sequenceNumber, false)

                onRemove: {
                    var viIndex = root._missionController.visualItemIndexForObject(delegateRoot.nodeObject)
                    if (viIndex > 0) {
                        root._missionController.removeVisualItem(viIndex)
                    }
                }

                onSelectNextNotReadyItem: {
                    for (var i = 0; i < root._missionController.visualItems.count; i++) {
                        var vmi = root._missionController.visualItems.get(i)
                        if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                            root._missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                            break
                        }
                    }
                }
            }
        }

        // ── GeoFence editor (single child of fence group) ──
        Component {
            id: fenceEditorComponent

            GeoFenceEditor {
                width: delegateRoot.width
                myGeoFenceController: root._geoFenceController
                flightMap: root.editorMap
            }
        }

        // ── Rally header / instructions ──
        Component {
            id: rallyHeaderComponent

            RallyPointEditorHeader {
                width: delegateRoot.width
                controller: root._rallyPointController
            }
        }

        // ── Rally point item editor ──
        Component {
            id: rallyItemComponent

            RallyPointItemEditor {
                width: delegateRoot.width
                rallyPoint: delegateRoot.nodeObject
                controller: root._rallyPointController
            }
        }

        // ── Transform editor (single child of transform group) ──
        Component {
            id: transformEditorComponent

            TransformEditor {
                width: delegateRoot.width
                missionController: root._missionController
            }
        }
    }
}
