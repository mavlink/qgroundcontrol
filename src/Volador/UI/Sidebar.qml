/****************************************************************************
 *
 * (c) 2026 Volador Aerospace. All rights reserved.
 *
 * Volador Ground Control Station (VGCS) - Modern Aerospace Navigation Rail Sidebar
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VoladorTheme 1.0
import VoladorComponents 1.0

VGCSNavigationRail {
    id: sidebarRoot

    property bool isExpanded: false

    signal pageSelected(int index, string pageUrl)

    onNavigationTriggered: function(index, routeId) {
        var target = ""
        switch (routeId) {
        case "flight": target = "showFlyView"; break;
        case "missions": target = "showPlanView"; break;
        case "vehicles": target = "qrc:/qml/SetupView.qml"; break;
        case "map": target = "qrc:/qml/MapSettings.qml"; break;
        case "telemetry": target = "qrc:/qml/AnalyzeView.qml"; break;
        case "video": target = "qrc:/qml/VideoSettings.qml"; break;
        case "logs": target = "qrc:/qml/AnalyzeView.qml"; break;
        case "settings": target = "qrc:/qml/AppSettings.qml"; break;
        default: target = "showFlyView"; break;
        }
        sidebarRoot.pageSelected(index, target)
    }
}
