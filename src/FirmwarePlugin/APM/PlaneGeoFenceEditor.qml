import QtQuick          2.2
import QtQuick.Controls 1.2

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

    Connections {
        target:         fenceAction
        onValueChanged: actionCombo.recalcSelection()
    }

    Connections {
        target:         fenceRetRally
        onValueChanged: actionCombo.recalcSelection()
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
            ListElement { text: qsTr("None"); actionValue: 0; retRallyValue: 0 }
            ListElement { text: qsTr("Report only"); actionValue: 2; retRallyValue: 0 }
            ListElement { text: qsTr("Fly to breach return point"); actionValue: 1; retRallyValue: 0 }
            ListElement { text: qsTr("Fly to breach return point (throttle control)"); actionValue: 3; retRallyValue: 0 }
            ListElement { text: qsTr("Fly to nearest rally point");  actionValue: 4; retRallyValue: 1 }
        }

        onActivated: {
            fenceAction.value = actionModel.get(index).actionValue
            fenceRetRally.value = actionModel.get(index).retRallyValue
        }

        function recalcSelection() {
            if (fenceAction.value != 0 && fenceRetRally.value == 1) {
                actionCombo.currentIndex = 4
            } else {
                switch (fenceAction.value) {
                case 0:
                    actionCombo.currentIndex = 0
                    break
                case 1:
                    actionCombo.currentIndex = 2
                    break
                case 2:
                    actionCombo.currentIndex = 1
                    break
                case 3:
                    actionCombo.currentIndex = 3
                    break
                case 4:
                    actionCombo.currentIndex = 4
                    break
                case 0:
                default:
                    actionCombo.currentIndex = 0
                    break
                }
            }
        }
    }

    Item { width:  1; height: 1 }

    QGCCheckBox {
        id:                 minAltFenceCheckBox
        text:               qsTr("Minimum altitude fence")
        checked:            fenceMinAlt.value > 0
        onClicked:          fenceMinAlt.value = checked ? 10 : 0
    }

    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        spacing:            _margin

        QGCLabel {
            anchors.baseline: fenceAltMinField.baseline
            text: qsTr("Min Altitude:")
        }

        FactTextField {
            id:                 fenceAltMinField
            showUnits:          true
            fact:               fenceMinAlt
            enabled:            minAltFenceCheckBox.checked
            width:              _editFieldWidth
        }
    }

    Item { width:  1; height: 1 }

    QGCCheckBox {
        id:                 maxAltFenceCheckBox
        text:               qsTr("Maximum altitude fence")
        checked:            fenceMaxAlt.value > 0
        onClicked:          fenceMaxAlt.value = checked ? 100 : 0
    }

    Row {
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.left:       parent.left
        spacing:            _margin

        QGCLabel {
            anchors.baseline: fenceAltMaxField.baseline
            text: qsTr("Max Altitude:")
        }

        FactTextField {
            id:                 fenceAltMaxField
            showUnits:          true
            fact:               fenceMaxAlt
            enabled:            maxAltFenceCheckBox.checked
            width:              _editFieldWidth
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

        onPolygonEditCompleted: geoFenceController.sendToVehicle()
    }
}
