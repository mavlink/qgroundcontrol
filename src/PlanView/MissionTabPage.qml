import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

import "PlanPanelLogic.js" as Logic

/// Mission tab: summary strip, item list and item inspector
ColumnLayout {
    id: root

    required property var planMasterController
    required property var editorMap
    property int incompleteCount: 0

    signal planTabRequested

    readonly property var   _missionController: planMasterController.missionController
    readonly property var   _appSettings:       QGroundControl.settingsManager.appSettings
    readonly property bool  _hasSelection:      _missionController.currentPlanViewItem !== null
    readonly property string layoutMode:        listInspector.layoutMode

    function openInspector() {
        listInspector.openInspector()
    }

    function closeInspector() {
        listInspector.closeInspector()
    }

    function _summaryText() {
        const distance = _missionController.missionPlannedDistance
        const parts = []
        if (!isNaN(distance)) {
            parts.push(QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(distance).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString)
        }
        parts.push(Logic.formatDuration(_missionController.missionTime))
        if (incompleteCount > 0) {
            parts.push(qsTr("%1 incomplete").arg(incompleteCount))
        }
        return parts.join(" · ")
    }

    objectName: "planPanel_page_mission"
    spacing:    ScreenTools.defaultFontPixelHeight * 0.3

    QGCPalette { id: qgcPal }

    RowLayout {
        Layout.fillWidth:       true
        Layout.leftMargin:      ScreenTools.defaultFontPixelWidth
        Layout.rightMargin:     ScreenTools.defaultFontPixelWidth
        Layout.topMargin:       ScreenTools.defaultFontPixelHeight * 0.3
        spacing:                ScreenTools.defaultFontPixelWidth

        QGCLabel {
            objectName:         "planPanel_missionSummary"
            Layout.fillWidth:   true
            text:               root._summaryText()
            color:              root.incompleteCount > 0 ? qgcPal.warningText : qgcPal.textSecondary
            font.pointSize:     ScreenTools.smallFontPointSize
            elide:              Text.ElideRight
        }

        QGCButton {
            objectName:     "planPanel_defaultAltitudeChip"
            small:          true
            text:           qsTr("Alt %1 %2").arg(root._appSettings.defaultMissionItemAltitude.valueString).arg(root._appSettings.defaultMissionItemAltitude.units)
            onClicked:      root.planTabRequested()
        }
    }

    PlanListInspector {
        id: listInspector

        objectName:         "planPanel_listInspector"
        Layout.fillWidth:   true
        Layout.fillHeight:  true
        hasSelection:       root._hasSelection
        listComponent:      missionListComponent
        inspectorComponent: missionInspectorComponent
    }

    Component {
        id: missionListComponent

        MissionItemList {
            missionController: root._missionController
        }
    }

    Component {
        id: missionInspectorComponent

        MissionItemInspector {
            missionController:  root._missionController
            map:                root.editorMap
            drillIn:            listInspector.layoutMode === "drillIn"
        }
    }
}
