import QtQuick

import QGroundControl
import QGroundControl.Controls

import "PlanPanelLogic.js" as Logic

/// Shows a list and an inspector for the selected list entry. When the height allows, the list sits above the inspector. On
/// short screens the list and the inspector take turns filling the whole area.
///
/// The list component exposes contentHeight and may emit itemActivated(). The inspector component may emit backRequested().
Item {
    id: root

    property Component listComponent
    property Component inspectorComponent
    property bool       hasSelection:       false
    property string     layoutModeOverride: ""      ///< "split" or "drillIn" to bypass the height check
    property real       userListHeight:     -1      ///< Set by dragging the divider, negative uses the height of the list content

    readonly property string layoutMode:        layoutModeOverride !== "" ? layoutModeOverride : Logic.layoutMode(height, ScreenTools.defaultFontPixelHeight)
    readonly property bool   inspectorVisible:  hasSelection && (layoutMode === "split" || _inspectorOpen)
    readonly property bool   listVisible:       layoutMode === "split" || !inspectorVisible

    property bool _inspectorOpen: false

    readonly property real _dividerHeight:      Math.max(ScreenTools.defaultFontPixelHeight * 0.8, ScreenTools.isMobile ? ScreenTools.minTouchPixels * 0.5 : 0)
    readonly property real _minListHeight:      ScreenTools.defaultFontPixelHeight * 4
    readonly property real _minInspectorHeight: ScreenTools.defaultFontPixelHeight * 8
    readonly property real _splitListHeight: {
        if (userListHeight >= 0) {
            return Logic.clampedListHeight(userListHeight, height, _dividerHeight, _minListHeight, _minInspectorHeight)
        }
        const contentHeight = listLoader.item ? listLoader.item.contentHeight : 0
        return Math.min(Math.max(contentHeight, ScreenTools.defaultFontPixelHeight * 6), height * 0.4)
    }

    function openInspector() {
        _inspectorOpen = true
    }

    function closeInspector() {
        _inspectorOpen = false
    }

    onHasSelectionChanged: {
        if (!hasSelection) {
            closeInspector()
        }
    }

    onLayoutModeChanged: {
        if (layoutMode === "drillIn" && hasSelection && !mainWindow.allowViewSwitch(0, false)) {
            openInspector()
        } else {
            closeInspector()
        }
    }

    QGCPalette { id: qgcPal }

    // Extends under the divider so only the top corners show as rounded
    Rectangle {
        objectName: "planPanel_listBackground"
        width:      root.width
        height:     listLoader.height + radius
        radius:     ScreenTools.defaultBorderRadius
        color:      qgcPal.windowCard
        visible:    root.layoutMode === "split" && root.inspectorVisible
    }

    Loader {
        id: listLoader

        objectName:         "planPanel_listLoader"
        x:                  0
        y:                  0
        width:              root.width
        height:             root.layoutMode === "split" ? (root.inspectorVisible ? root._splitListHeight : root.height) : root.height
        visible:            root.listVisible
        sourceComponent:    root.listComponent
    }

    Connections {
        target:                 listLoader.item
        ignoreUnknownSignals:   true

        function onItemActivated() {
            root.openInspector()
        }
    }

    Rectangle {
        id: divider

        objectName: "planPanel_listInspectorDivider"
        y:          root._splitListHeight
        width:      root.width
        height:     root._dividerHeight
        color:      qgcPal.windowCard
        visible:    root.layoutMode === "split" && root.inspectorVisible

        Rectangle {
            anchors.top:    parent.top
            width:          parent.width
            height:         1
            color:          qgcPal.borderSubtle
        }

        Rectangle {
            anchors.centerIn:   parent
            width:              ScreenTools.defaultFontPixelHeight * 2.5
            height:             Math.max(ScreenTools.defaultFontPixelHeight * 0.25, 3)
            radius:             height / 2
            color:              dragArea.pressed ? qgcPal.accent : qgcPal.textSecondary
        }

        MouseArea {
            id: dragArea

            property real startY:       0
            property real startHeight:  0

            anchors.fill:   parent
            cursorShape:    Qt.SizeVerCursor

            onPressed: (mouse) => {
                startY = mapToItem(root, mouse.x, mouse.y).y
                startHeight = root._splitListHeight
            }

            onPositionChanged: (mouse) => {
                if (pressed) {
                    root.userListHeight = Logic.clampedListHeight(startHeight + mapToItem(root, mouse.x, mouse.y).y - startY,
                                                                  root.height, root._dividerHeight, root._minListHeight, root._minInspectorHeight)
                }
            }

            onDoubleClicked: root.userListHeight = -1
        }
    }

    Loader {
        id: inspectorLoader

        objectName:         "planPanel_inspectorLoader"
        x:                  0
        y:                  root.layoutMode === "split" ? root._splitListHeight + root._dividerHeight : 0
        width:              root.width
        height:             root.height - y
        visible:            root.inspectorVisible
        sourceComponent:    root.inspectorComponent
    }

    Connections {
        target:                 inspectorLoader.item
        ignoreUnknownSignals:   true

        function onBackRequested() {
            if (mainWindow.allowViewSwitch()) {
                root.closeInspector()
            }
        }
    }
}
