import QtQuick          2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts  1.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FactSystem    1.0

Column {
    id:         editorColumn
    width:      availableWidth
    spacing:    _margin

    //property real availableWidth - Available width for control - Must be passed in from Loader

    readonly property real _margin:         ScreenTools.defaultFontPixelWidth / 2
    readonly property real _editFieldWidth: Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 15)

    property Fact fenceAction:     factController.getParameterFact(-1, "FENCE_ACTION")
    property Fact fenceMinAlt:     factController.getParameterFact(-1, "FENCE_MINALT")
    property Fact fenceMaxAlt:     factController.getParameterFact(-1, "FENCE_MAXALT")
    property Fact fenceRetAlt:     factController.getParameterFact(-1, "FENCE_RETALT")
    property Fact fenceAutoEnable: factController.getParameterFact(-1, "FENCE_AUTOENABLE")
    property Fact fenceRetRally:   factController.getParameterFact(-1, "FENCE_RET_RALLY")

    FactPanelController {
        id:         factController
        factPanel:  qgcView.viewPanel
    }

    QGCLabel { text: qsTr("Fence Settings:") }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         1
        color:          qgcPal.text
    }

    QGCLabel { text: qsTr("Action on fence breach:") }

    QGCComboBox {
        id:                 actionCombo
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      parent.right
        model: ListModel {
            id: actionModel
            ListElement { text: qsTr("None"); actionValue: 0 }
            ListElement { text: qsTr("Report only"); actionValue: 2 }
            ListElement { text: qsTr("Fly to return point"); actionValue: 1 }
            ListElement { text: qsTr("Fly to return point (throttle control)"); actionValue: 3 }
        }

        onActivated: fenceAction.rawValue = actionModel.get(index).actionValue
        Component.onCompleted: recalcSelection()

        Connections {
            target:         fenceAction
            onValueChanged: actionCombo.recalcSelection()
        }

        function recalcSelection() {
            for (var i=0; i<actionModel.count; i++) {
                if (actionModel.get(i).actionValue == fenceAction.rawValue) {
                    actionCombo.currentIndex = i
                    return
                }
            }
            actionCombo.currentIndex = 0
        }
    }

    ExclusiveGroup { id: returnLocationRadioGroup }

    property bool _returnPointUsed: fenceAction.rawValue == 1 || fenceAction.rawValue == 3

    QGCRadioButton {
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        text:               qsTr("Fly to breach return point")
        checked:            fenceRetRally.rawValue != 1
        enabled:            _returnPointUsed
        exclusiveGroup:     returnLocationRadioGroup
        onClicked:          fenceRetRally.rawValue = 0
    }

    QGCRadioButton {
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        text:               qsTr("Fly to nearest rally point")
        checked:            fenceRetRally.rawValue == 1
        enabled:            _returnPointUsed
        exclusiveGroup:     returnLocationRadioGroup
        onClicked:          fenceRetRally.rawValue = 1
    }

    Item { width:  1; height: 1 }

    GridLayout {
        anchors.left:   parent.left
        anchors.right:  parent.right
        columnSpacing:  ScreenTools.defaultFontPixelWidth
        columns:        2

        QGCCheckBox {
            id:                 minAltFenceCheckBox
            text:               qsTr("Min altitude:")
            checked:            fenceMinAlt.value > 0
            onClicked:          fenceMinAlt.value = checked ? 10 : 0
        }

        FactTextField {
            id:                 fenceAltMinField
            Layout.fillWidth:   true
            showUnits:          true
            fact:               fenceMinAlt
            enabled:            minAltFenceCheckBox.checked
        }

        QGCCheckBox {
            id:                 maxAltFenceCheckBox
            text:               qsTr("Max altitude:")
            checked:            fenceMaxAlt.value > 0
            onClicked:          fenceMaxAlt.value = checked ? 100 : 0
        }

        FactTextField {
            id:                 fenceAltMaxField
            Layout.fillWidth:   true
            showUnits:          true
            fact:               fenceMaxAlt
            enabled:            maxAltFenceCheckBox.checked
        }
    }

    Item { width:  1; height: 1 }

    QGCLabel { text: qsTr("Breach return point:") }

    QGCLabel {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      parent.right
        wrapMode:           Text.WordWrap
        text:               qsTr("Click in map to set breach return location.")
        font.pointSize:     ScreenTools.smallFontPointSize
    }

    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        spacing:            _margin

        QGCLabel {
            anchors.baseline: retAltField.baseline
            text: qsTr("Return Alt:")
        }

        FactTextField {
            id:         retAltField
            showUnits:  true
            fact:       fenceRetAlt
            width:      _editFieldWidth
        }
    }

    Item { width:  1; height: 1 }

    FactCheckBox {
        text: qsTr("Auto-Enable after takeoff")
        fact: fenceAutoEnable
    }

    QGCLabel {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        anchors.right:      parent.right
        wrapMode:           Text.WordWrap
        font.pointSize:     ScreenTools.smallFontPointSize
        text:               qsTr("If checked, the aircraft will start with the fence disabled. After an autonomous takeoff completes the fences will automatically enable. When the autonomous mission arrives at a landing waypoint the fence automatically disables.")
    }

    /*
    FENCE_ACTION - the action to take on fence breach. This defaults to zero which disables geo-fencing. Set it to 1 to enable geo-fencing and fly to the return point on fence breach. Set to 2 to report a breach to the GCS but take no other action. Set to 3 to have the plane head to the return point on breach, but the pilot will maintain manual throttle control in this case.
    FENCE_MINALT - the minimum altitude in meters. If this is zero then you will not have a minimum altitude.
    FENCE_MAXALT - the maximum altitude in meters. If this is zero then you will not have a maximum altitude.
    FENCE_CHANNEL - the RC input channel to watch for enabling the geo-fence. This defaults to zero, which disables geo-fencing. You should set it to a spare RC input channel that is connected to a two position switch on your transmitter. Fencing will be enabled when this channel goes above a PWM value of 1750. If your transmitter supports it it is also a good idea to enable audible feedback when this channel is enabled (a beep every few seconds), so you can tell if the fencing is enabled without looking down.
    FENCE_TOTAL - the number of points in your fence (the return point plus the enclosed boundary). This should be set for you by the planner when you create the fence.
    FENCE_RETALT - the altitude the aircraft will fly at when flying to the return point and when loitering at the return point (in meters). Note that when FENCE_RET_RALLY is set to 1 this parameter is ignored and the loiter altitude of the closest Rally Point is used instead. If this parameter is zero and FENCE_RET_RALLY is also zero, the midpoint of the FENCE_MAXALT and FENCE_MINALT parameters is used as the return altitude.
    FENCE_AUTOENABLE - if set to 1, the aircraft will boot with the fence disabled. After an autonomous takeoff completes the fences will automatically enable. When the autonomous mission arrives at a landing waypoint the fence automatically disables.
    FENCE_RET_RALLY - if set to 1 the aircraft will head to the nearest Rally Point rather than the fence return point when the fence is breached. Note that the loiter altitude of the Rally Point is used as the return altitude.
    */

    QGCMapPolygonControls {
        anchors.left:   parent.left
        anchors.right:  parent.right
        flightMap:      editorMap
        polygon:        geoFenceController.polygon
        sectionLabel:   qsTr("Fence Polygon:")

        onPolygonEditCompleted: geoFenceController.validateBreachReturn()
    }
}
