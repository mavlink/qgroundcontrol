import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    required property var editorMap
    required property var planMasterController

    signal editingLayerChangeRequested(int layer)

    id: root

    property var  _missionController: planMasterController.missionController
    property real _toolsMargin:       ScreenTools.defaultFontPixelWidth * 0.75

    readonly property bool _useNewPlanPanel: QGroundControl.settingsManager.planViewSettings.useNewPlanPanel.rawValue

    function selectNextNotReady() {
        for (var i = 0; i < _missionController.visualItems.count; i++) {
            var vmi = _missionController.visualItems.get(i)
            if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                _missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                break
            }
        }
    }

    QGCPalette { id: qgcPal }

    Rectangle {
        id:             rightPanelBackground
        anchors.fill:   parent
        color:          qgcPal.window
        opacity:        0.85
        visible:        !root._useNewPlanPanel
    }


    // Open/Close panel
    Item {
        id:                     panelOpenCloseButton
        anchors.right:          parent.left
        anchors.verticalCenter: parent.verticalCenter
        // The tabbed panel has its tab pills at the top of this edge so the button moves to the bottom
        anchors.verticalCenterOffset: root._useNewPlanPanel ? parent.height / 2 - height / 2 - _toolsMargin : 0
        width:                  toggleButtonRect.width - toggleButtonRect.radius * (root._useNewPlanPanel ? 2 : 1)
        height:                 toggleButtonRect.height
        clip:                   true

        property bool _expanded: root.anchors.right == root.parent.right

        Rectangle {
            id:             toggleButtonRect
            // The tabbed panel styles the button like its tab pills, with the inner side tucked under the panel
            width:          root._useNewPlanPanel ? ScreenTools.defaultFontPixelHeight * 2.2 : ScreenTools.defaultFontPixelWidth * 2.25
            height:         root._useNewPlanPanel ? ScreenTools.defaultFontPixelHeight * 2.2 : width * 3
            radius:         ScreenTools.defaultBorderRadius
            color:          root._useNewPlanPanel ? qgcPal.windowPanel : rightPanelBackground.color
            opacity:        root._useNewPlanPanel ? 1 : rightPanelBackground.opacity
            border.width:   root._useNewPlanPanel ? 1 : 0
            border.color:   qgcPal.borderSubtle

            QGCLabel {
                id:                 toggleButtonLabel
                anchors.centerIn:   parent
                anchors.horizontalCenterOffset: root._useNewPlanPanel ? -toggleButtonRect.radius : 0
                text:               panelOpenCloseButton._expanded ? ">" : "<"
                color:              qgcPal.textPrimary
            }

        }

        QGCMouseArea {
            anchors.fill: parent

            onClicked: {
                if (panelOpenCloseButton._expanded) {
                    // Close panel
                    root.anchors.right = undefined
                    root.anchors.left = root.parent.right
                } else {
                    // Open panel
                    root.anchors.left = undefined
                    root.anchors.right = root.parent.right
                }
            }
        }
    }

    //-------------------------------------------------------
    // Right Panel Controls
    Item {
        anchors.fill: rightPanelBackground

        DeadMouseArea {
            anchors.fill:   parent
        }

        Loader {
            id:             planTreeLoader
            anchors.fill:   parent
            active:         !root._useNewPlanPanel

            sourceComponent: PlanTreeView {
                objectName:             "planView_planTree"
                editorMap:              root.editorMap
                planMasterController:   root.planMasterController
                onEditingLayerChangeRequested: (layer) => root.editingLayerChangeRequested(layer)
            }
        }

        Loader {
            id:             planPanelLoader
            anchors.fill:   parent
            active:         root._useNewPlanPanel

            sourceComponent: PlanPanel {
                editorMap:              root.editorMap
                planMasterController:   root.planMasterController
                onEditingLayerChangeRequested: (layer) => root.editingLayerChangeRequested(layer)
            }
        }
    }

    /// Called when the user picks a mission item on the map
    function showItemInspector() {
        if (planPanelLoader.item) {
            planPanelLoader.item.showItemInspector()
        }
    }

    function selectLayer(nodeType) {
        // Ensure panel is open
        if (!panelOpenCloseButton._expanded) {
            root.anchors.left = undefined
            root.anchors.right = root.parent.right
        }
        if (planPanelLoader.item) {
            planPanelLoader.item.selectLayer(nodeType)
        } else if (planTreeLoader.item) {
            planTreeLoader.item.selectLayer(nodeType)
        }
    }
}
