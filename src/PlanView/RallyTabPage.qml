import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PlanPanelLogic.js" as Logic

/// Rally Points tab: the list of points and the inspector of the current one
ColumnLayout {
    id: root

    required property var planMasterController

    readonly property var   controller:     planMasterController.rallyPointController
    readonly property var   points:         controller.points
    readonly property var   current:        controller.currentRallyPoint
    readonly property bool  supported:      controller.supported
    readonly property int   currentIndex:   indexOfPoint(current)

    function indexOfPoint(point) {
        for (let i = 0; i < points.count; i++) {
            if (points.get(i) === point) {
                return i
            }
        }
        return -1
    }

    function select(point) {
        controller.currentRallyPoint = point
    }

    function step(delta) {
        if (!mainWindow.allowViewSwitch()) {
            return
        }
        const index = Logic.steppedIndex(currentIndex, delta, points.count)
        if (index >= 0) {
            select(points.get(index))
        }
    }

    function deleteCurrent() {
        if (current) {
            controller.removePoint(current)
        }
    }

    function openInspector() {
        listInspector.openInspector()
    }

    objectName: "planPanel_page_rally"
    spacing:    ScreenTools.defaultFontPixelHeight * 0.3

    QGCPalette { id: qgcPal }

    Connections {
        target: root.controller

        function onCurrentRallyPointChanged() {
            if (root.controller.currentRallyPoint) {
                root.openInspector()
            }
        }
    }

    QGCLabel {
        objectName:         "planPanel_rallyHeader"
        Layout.fillWidth:   true
        Layout.margins:     ScreenTools.defaultFontPixelWidth
        wrapMode:           Text.WordWrap
        color:              qgcPal.textSecondary
        font.pointSize:     ScreenTools.smallFontPointSize
        text:               root.supported ?
                                qsTr("Rally Points provide alternate landing points when performing a Return to Launch (RTL). Click in the map to add a point.") :
                                qsTr("This vehicle does not support Rally Points.")
    }

    PlanListInspector {
        id: listInspector

        objectName:         "planPanel_rallyListInspector"
        Layout.fillWidth:   true
        Layout.fillHeight:  true
        visible:            root.supported
        hasSelection:       root.current !== null
        listComponent:      rallyListComponent
        inspectorComponent: rallyInspectorComponent
    }

    Component {
        id: rallyListComponent

        RallyPointList {
            page: root
        }
    }

    Component {
        id: rallyInspectorComponent

        RallyPointInspector {
            page:       root
            drillIn:    listInspector.layoutMode === "drillIn"
        }
    }
}
