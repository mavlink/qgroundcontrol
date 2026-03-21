import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// ToolIndicatorPage
//      The base control for all Toolbar Indicator drop down pages. It supports a normal and expanded view.

RowLayout {
    id:         control
    spacing:    ScreenTools.defaultFontPixelWidth

    property bool       showExpand:         false   // Controls whether the expand widget is shown or not
    property bool       waitForParameters:  false   // UI won't show until parameters are ready
    property bool       expandedComponentWaitForParameters: false   // If true, the expanded component won't show until parameters are ready
    property Component  contentComponent            // Item for the normal view portion of the page
    property Component  expandedComponent           // Item for the expanded portion of the page
    property var        pageProperties              // Allows you to share a QtObject full of properties between pages

    // These properties are bound by the MainRoowWindow loader
    property bool expanded: false
    property var  drawer

    property var    activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property bool   parametersReady:    QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable

    property bool _showMainComponent: !waitForParameters || parametersReady
    property bool _showExpand: showExpand && expandedComponent !== undefined
    property bool _expandedParametersReady: !expandedComponentWaitForParameters || parametersReady
    property string _waitingForParamsText: activeVehicle && activeVehicle.parameterManager.parameterDownloadSkipped ? qsTr("Parameters not available") : qsTr("Waiting for parameters...")

    QGCLabel {
        text:       control._waitingForParamsText
        visible:    waitForParameters && !parametersReady
    }

    Loader {
        id:                 contentItemLoader
        Layout.alignment:   Qt.AlignTop
        sourceComponent:    _showMainComponent ? contentComponent : undefined

        property var pageProperties: control.pageProperties
    }

    Rectangle {
        id:                     divider
        Layout.preferredWidth:  visible ? 1 : -1
        Layout.fillHeight:      true
        color:                  QGroundControl.globalPalette.groupBorder
        visible:                expanded
    }

    QGCLabel {
        text:               activeVehicle && activeVehicle.parameterManager.parameterDownloadSkipped ? qsTr("Parameters not available") : qsTr("Waiting for parameters...")
        visible:            expanded && !_expandedParametersReady
        Layout.alignment:   Qt.AlignTop
    }

    Loader {
        id:                     expandedItemLoader
        Layout.alignment:       Qt.AlignTop
        Layout.preferredWidth:  visible ? -1 : 0
        visible:                expanded && _expandedParametersReady
        sourceComponent:        expanded && _expandedParametersReady ? expandedComponent : undefined

        property var pageProperties: control.pageProperties
    }
}
