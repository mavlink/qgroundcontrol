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

    objectName: "planView_rightPanel"

    /// Width of the open/close button which hangs off the left edge of the panel
    readonly property real toggleWidth: panelOpenCloseButton.width

    property real _toolsMargin:       ScreenTools.defaultFontPixelWidth * 0.75

    function _openPanel() {
        root.anchors.left = undefined
        root.anchors.right = root.parent.right
    }

    function _closePanel() {
        root.anchors.right = undefined
        root.anchors.left = root.parent.right
    }

    QGCPalette { id: qgcPal }

    // Open/Close panel
    Item {
        id:                     panelOpenCloseButton
        objectName:             "planView_rightPanelToggle"
        anchors.right:          parent.left
        anchors.verticalCenter: parent.verticalCenter
        // The panel has its tab pills at the top of this edge so the button sits at the bottom
        anchors.verticalCenterOffset: parent.height / 2 - height / 2 - _toolsMargin
        width:                  toggleButtonRect.width - toggleButtonRect.radius * 2
        height:                 toggleButtonRect.height
        clip:                   true

        property bool _expanded: root.anchors.right == root.parent.right

        Rectangle {
            id:             toggleButtonRect
            // Styled like the panel's tab pills, with the inner side tucked under the panel
            width:          ScreenTools.defaultFontPixelHeight * 2.2
            height:         width
            radius:         ScreenTools.defaultBorderRadius
            color:          qgcPal.windowPanel
            border.width:   1
            border.color:   qgcPal.borderSubtle

            QGCLabel {
                id:                 toggleButtonLabel
                anchors.centerIn:   parent
                anchors.horizontalCenterOffset: -toggleButtonRect.radius
                text:               panelOpenCloseButton._expanded ? ">" : "<"
                color:              qgcPal.textPrimary
            }

        }

        QGCMouseArea {
            anchors.fill: parent

            onClicked: {
                if (panelOpenCloseButton._expanded) {
                    root._closePanel()
                } else {
                    root._openPanel()
                }
            }
        }
    }

    //-------------------------------------------------------
    // Right Panel Controls
    Item {
        anchors.fill: parent

        DeadMouseArea {
            anchors.fill:   parent
        }

        PlanPanel {
            id:                     planPanel
            anchors.fill:           parent
            editorMap:              root.editorMap
            planMasterController:   root.planMasterController
            onEditingLayerChangeRequested: (layer) => root.editingLayerChangeRequested(layer)
        }
    }

    /// Called when the user picks a mission item on the map
    function showItemInspector() {
        _openPanel()
        planPanel.showItemInspector()
    }
}
