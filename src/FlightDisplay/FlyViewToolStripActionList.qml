/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

ToolStripActionList {
    id: _root

    signal displayPreFlightChecklist

    model: [
        ToolStripAction {
            property bool _is3DViewOpen:            viewer3DWindow.isOpen
            property bool   _viewer3DEnabled:       QGroundControl.settingsManager.viewer3DSettings.enabled.rawValue

            id: view3DIcon
            visible: _viewer3DEnabled
            text:           qsTr("3D View")
            iconSource:     "/qmlimages/Viewer3D/City3DMapIcon.svg"
            onTriggered:{
                if(_is3DViewOpen === false){
                    viewer3DWindow.open()
                }else{
                    viewer3DWindow.close()
                }
            }

            on_Is3DViewOpenChanged: {
                if(_is3DViewOpen === true){
                    view3DIcon.iconSource =     "/qmlimages/PaperPlane.svg"
                    text=           qsTr("Fly")
                }else{
                    iconSource =     "/qmlimages/Viewer3D/City3DMapIcon.svg"
                    text =           qsTr("3D View")
                }
            }
        },
        ToolStripAction {
            text:       qsTr("Plan")
            iconSource: "/qmlimages/Plan.svg"

            onTriggered: {
                mainWindow.showPlanView()
            }
        },
        PreFlightCheckListShowAction { onTriggered: displayPreFlightChecklist() },
        ToolStripAction {
            id: modeAction

            property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            text:       _activeVehicle ? shortFlightMode(_activeVehicle.flightMode) : qsTr("Mode")
            iconSource: "/qmlimages/FlightModesComponentIcon.png"
            visible:    _activeVehicle && _activeVehicle.flightModeSetAvailable
            enabled:    visible

            function flightModeDisplayText(mode) {
                if (!mode || mode.length === 0) {
                    return qsTr("Mode")
                }

                var prefix = ""
                var displayMode = mode
                var normalized = mode.toLowerCase()
                if (normalized.indexOf("quadplane ") === 0) {
                    prefix = qsTr("QuadPlane") + " "
                    displayMode = mode.substr(10)
                    normalized = displayMode.toLowerCase()
                }
                if (normalized === "manual") {
                    return prefix + qsTr("Manual")
                }
                if (normalized === "stabilize" || normalized === "stabilized") {
                    return prefix + qsTr("Stabilize")
                }
                if (normalized === "alt hold" || normalized === "althold" || normalized === "altitude hold") {
                    return prefix + qsTr("Alt Hold")
                }
                if (normalized === "loiter") {
                    return prefix + qsTr("Loiter")
                }
                if (normalized.indexOf("follow") !== -1) {
                    return prefix + qsTr("Follow")
                }
                if (normalized.indexOf("guided") !== -1) {
                    return prefix + qsTr("Guided")
                }
                if (normalized.indexOf("auto") !== -1) {
                    return prefix + qsTr("Auto")
                }
                if (normalized === "rtl") {
                    return prefix + qsTr("RTL")
                }
                if (normalized === "land") {
                    return prefix + qsTr("Land")
                }
                if (normalized === "takeoff") {
                    return prefix + qsTr("Takeoff")
                }
                if (normalized.indexOf("hold") !== -1) {
                    return prefix + qsTr("Hold")
                }
                if (normalized.indexOf("position") !== -1 || normalized.indexOf("pos") !== -1) {
                    return prefix + qsTr("Position")
                }
                if (normalized === "acro") {
                    return prefix + qsTr("Acro")
                }
                if (normalized === "brake") {
                    return prefix + qsTr("Brake")
                }
                if (normalized === "circle") {
                    return prefix + qsTr("Circle")
                }
                return prefix + displayMode
            }

            function shortFlightMode(mode) {
                var translated = flightModeDisplayText(mode)
                return translated.length > 10 ? translated.split(" ")[0] : translated
            }

            dropPanelComponent: Component {
                Rectangle {
                    id:     flightModePanel
                    width:  Math.max(ScreenTools.defaultFontPixelWidth * 19, ScreenTools.minTouchPixels * 3.0)
                    height: modeColumn.implicitHeight + (_panelMargin * 2)
                    color:  "transparent"

                    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
                    property real _panelMargin: ScreenTools.defaultFontPixelWidth * 0.80

                    QGCPalette { id: qgcPal }

                    ColumnLayout {
                        id:                 modeColumn
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        parent.top
                        anchors.margins:    flightModePanel._panelMargin
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.35

                        QGCLabel {
                            Layout.fillWidth:   true
                            text:               qsTr("Flight Mode")
                            color:              qgcPal.buttonText
                            font.bold:          true
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Repeater {
                            model: flightModePanel._activeVehicle ? flightModePanel._activeVehicle.flightModes : []

                            QGCButton {
                                Layout.fillWidth:   true
                                text:               modeAction.flightModeDisplayText(modelData)
                                enabled:            flightModePanel._activeVehicle && flightModePanel._activeVehicle.flightModeSetAvailable
                                highlighted:        flightModePanel._activeVehicle && flightModePanel._activeVehicle.flightMode === modelData

                                onClicked: {
                                    if (flightModePanel._activeVehicle) {
                                        flightModePanel._activeVehicle.flightMode = modelData
                                    }
                                    dropPanel.hide()
                                }
                            }
                        }
                    }
                }
            }
        },
        GuidedActionTakeoff { },
        GuidedActionLand { },
        GuidedActionRTL { },
        GuidedActionPause { },
        FlyViewAdditionalActionsButton { },
        GuidedActionGripper { }
    ]
}
