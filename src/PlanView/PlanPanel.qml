import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.PlanView

import "PlanPanelLogic.js" as Logic

/// Tabbed Plan View panel: Plan, Mission, GeoFence and Rally Points. The tab pills hang off the left edge of the panel.
Item {
    id: root

    required property var editorMap
    required property var planMasterController

    signal editingLayerChangeRequested(int layer)

    property string currentTab: Logic.TabPlan

    readonly property var   _missionController:     planMasterController.missionController
    readonly property var   _visualItems:           _missionController.visualItems
    readonly property var   _layerForTab: ({
        "plan":     PlanEditLayers.layerPlan,
        "mission":  PlanEditLayers.layerMission,
        "fence":    PlanEditLayers.layerFence,
        "rally":    PlanEditLayers.layerRally
    })
    readonly property var   tabModel: Logic.tabModel({ incompleteCount: _incompleteCount })

    property int _incompleteCount:  0
    property int _lastItemCount:    0

    /// Same node type names the tree view groups use
    function selectLayer(nodeType) {
        switch (nodeType) {
        case "missionGroup":
            selectTab(Logic.TabMission)
            break
        case "fenceGroup":
            selectTab(Logic.TabFence)
            break
        case "rallyGroup":
            selectTab(Logic.TabRally)
            break
        }
    }

    function selectTab(tab) {
        if (tab === currentTab) {
            return
        }
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        _setTab(tab)
    }

    /// Called when the user picks a mission item on the map
    function showItemInspector() {
        if (currentTab !== Logic.TabMission) {
            _setTab(Logic.TabMission)
        }
        missionPage.openInspector()
    }

    function _setTab(tab) {
        currentTab = tab
        editingLayerChangeRequested(_layerForTab[tab])
    }

    function _resetTab() {
        _lastItemCount = _visualItems ? _visualItems.count : 0
        _setTab(Logic.defaultTab(planMasterController.containsItems))
        missionPage.closeInspector()
    }

    function _updateIncompleteCount() {
        const states = []
        for (let i = 0; i < readyStateTracker.count; i++) {
            const tracked = readyStateTracker.objectAt(i)
            if (tracked) {
                states.push(tracked.readyState)
            }
        }
        _incompleteCount = Logic.incompleteItemCount(states, VisualMissionItem.ReadyForSave)
    }

    objectName: "planPanel"

    Component.onCompleted: _resetTab()

    QGCPalette { id: qgcPal }

    Instantiator {
        id: readyStateTracker

        model: root._visualItems

        delegate: QtObject {
            required property var object

            readonly property int readyState: object ? object.readyForSaveState : VisualMissionItem.ReadyForSave

            onReadyStateChanged: Qt.callLater(root._updateIncompleteCount)
        }

        onObjectAdded: Qt.callLater(root._updateIncompleteCount)
        onObjectRemoved: Qt.callLater(root._updateIncompleteCount)
    }

    Connections {
        target: root._missionController

        // Plan clears reset the mission before the fence and rally points, so wait until all of them are done
        function onVisualItemsReset() {
            Qt.callLater(root._resetTab)
        }
    }

    Connections {
        target: root._visualItems

        function onCountChanged() {
            const count = root._visualItems.count
            if (count > root._lastItemCount) {
                const tab = Logic.tabAfterItemAdded(root.currentTab)
                if (tab !== root.currentTab) {
                    root._setTab(tab)
                }
            }
            root._lastItemCount = count
        }
    }

    Rectangle {
        id: card

        objectName:     "planPanel_card"
        anchors.fill:   parent
        radius:         ScreenTools.defaultBorderRadius * 2
        color:          qgcPal.windowPanel
        border.color:   qgcPal.borderSubtle
    }

    StackLayout {
        id: pages

        objectName:         "planPanel_pages"
        anchors.fill:       parent
        anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.5
        currentIndex:       Math.max(Logic.tabIds.indexOf(root.currentTab), 0)

        PlanTabPage {
            planMasterController:   root.planMasterController
            editorMap:              root.editorMap

            onMissionTabRequested: root._setTab(Logic.TabMission)
        }

        MissionTabPage {
            id: missionPage

            planMasterController:   root.planMasterController
            editorMap:              root.editorMap
            incompleteCount:        root._incompleteCount

            onPlanTabRequested: root.selectTab(Logic.TabPlan)
        }

        FenceTabPage {
            planMasterController:   root.planMasterController
            editorMap:              root.editorMap
        }

        RallyTabPage {
            planMasterController: root.planMasterController
        }
    }

    PlanPanelTabBar {
        id: tabBar

        objectName:     "planPanel_tabBar"
        // Hangs off the left edge with its inner side tucked under the card
        x:              -width + ScreenTools.defaultBorderRadius * 2
        y:              ScreenTools.defaultFontPixelHeight
        z:              -1
        model:          root.tabModel
        currentTab:     root.currentTab
        style:          Logic.tabStyle(root.height - y, ScreenTools.defaultFontPixelHeight)

        onTabClicked: (tab) => root.selectTab(tab)
    }
}
