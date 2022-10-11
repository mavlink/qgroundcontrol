

/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.12

import QtLocation 5.3
import QtPositioning 5.3
import QtQuick.Window 2.2
import QtQml.Models 2.1

import QGroundControl 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Airspace 1.0
import QGroundControl.Airmap 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.Controls 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Vehicle 1.0

// This is the ui overlay layer for the widgets/tools for Fly View
Item {
    id: _root

    property var parentToolInsets
    property var totalToolInsets: _totalToolInsets
    property var mapControl

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var _planMasterController: globals.planMasterControllerFlyView
    property var _missionController: _planMasterController.missionController
    property var _geoFenceController: _planMasterController.geoFenceController
    property var _rallyPointController: _planMasterController.rallyPointController
    property var _guidedController: globals.guidedControllerFlyView
    property real _margins: ScreenTools.defaultFontPixelWidth / 2
    property real _toolsMargin: ScreenTools.defaultFontPixelWidth * 0.75
    property rect _centerViewport: Qt.rect(0, 0, width, height)
    property real _rightPanelWidth: ScreenTools.defaultFontPixelWidth * 30

    property real _heading: _activeVehicle ? _activeVehicle.heading.rawValue : 0
    property real _indicatorsHeight: ScreenTools.defaultFontPixelHeight

    QGCToolInsets {
        id: _totalToolInsets
        leftEdgeTopInset: toolStrip.leftInset
        leftEdgeCenterInset: toolStrip.leftInset
        leftEdgeBottomInset: parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset: parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset: parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset: parentToolInsets.rightEdgeBottomInset
        topEdgeLeftInset: parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset: parentToolInsets.topEdgeCenterInset
        topEdgeRightInset: parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset: parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset: mapScale.centerInset
        bottomEdgeRightInset: 0
    }

    Rectangle {
        id: compassBar
        height: ScreenTools.defaultFontPixelHeight * 4 // * 1.5
        width: ScreenTools.defaultFontPixelWidth * 100 // * 50
        color: "#70000000" // #DEDEDE 00000080
        radius: 4 // 2
        clip: true
        anchors.top: headingIndicator.bottom
        anchors.topMargin: -headingIndicator.height / 2
        anchors.horizontalCenter: parent.horizontalCenter
        Repeater {
            model: 720
            QGCLabel {
                function _normalize(degrees) {
                    var a = degrees % 360
                    if (a < 0)
                        a += 360
                    return a
                }
                property bool _isMainDirection: false
                property int _startAngle: modelData + 180 + _heading
                property int _angle: _normalize(_startAngle)
                anchors.verticalCenter: parent.verticalCenter
                x: visible ? ((modelData * (compassBar.width / 360)) - (width * 0.5)) : 0
                visible: _angle % 45 == 0
                color: "white" // "#000000" 75505565 color of the compass text
                font.pointSize: _isMainDirection ? 16 : ScreenTools.mediumFontPointSize //smallFontPointSize
                font.weight: _isMainDirection ? Font.DemiBold : Font.Normal
                text: {
                    switch (_angle) {
                    case 0:
                        _isMainDirection = true
                        return "N"
                    case 45:
                        _isMainDirection = false
                        return "45" //NE
                    case 90:
                        _isMainDirection = true
                        return "E"
                    case 135:
                        _isMainDirection = false
                        return "135" //SE
                    case 180:
                        _isMainDirection = true
                        return "S"
                    case 225:
                        _isMainDirection = false
                        return "225" //SW
                    case 270:
                        _isMainDirection = true
                        return "W"
                    case 315:
                        _isMainDirection = false
                        return "315" //NW
                    }
                    return "test"
                }
            }
        }

        Repeater {
            model: 720
            Image {
                function _normalize(degrees) {
                    var a = degrees % 360
                    if (a < 0)
                        a += 360
                    return a
                }
                property int _startAngle: modelData + 180 + _heading
                property int _angle: _normalize(_startAngle)

                fillMode:                   Image.PreserveAspectFit
                x: visible ? ((modelData * (compassBar.width / 360)) - (width * 0.5)) : 0
                anchors.verticalCenter: parent.verticalCenter
                visible: _angle % 45 == 0
                height: 200
                width: 50
                source: {
                    switch (_angle) {
                    case 0:     return ""
                    case 45:    return "/qmlimages/ui/thinLine.png"
                    case 90:    return ""
                    case 135:   return "/qmlimages/ui/thinLine.png"
                    case 180:   return ""
                    case 225:   return "/qmlimages/ui/thinLine.png"
                    case 270:   return ""
                    case 315:   return "/qmlimages/ui/thinLine.png"
                    }
                    return ""
                }

                //height:                     _indicatorsHeight
                //width:                      height
                //sourceSize.height:          height
                //anchors.top:                compassBar.bottom
                //anchors.topMargin:          -height / 2
                //anchors.horizontalCenter:   parent.horizontalCenter
            }
        }

    }
    Rectangle {
        id: headingIndicator
        height: ScreenTools.defaultFontPixelHeight * 2 // -
        width: ScreenTools.defaultFontPixelWidth * 4 * 2 // * 4
        color: qgcPal.windowShadeDark // background color of the heading indicator, the number that shows the degree
        anchors.top: parent.top
        anchors.topMargin: _toolsMargin // -
        anchors.horizontalCenter: parent.horizontalCenter
        QGCLabel {
            text: _heading
            color: qgcPal.text
            font.pointSize: 12 // ScreenTools.mediumFontPointSize smallFontPointSize
            anchors.centerIn: parent
        }
    }
    Image {
        id: compassArrowIndicator
        height: _indicatorsHeight * 1.5
        width: height
        source: "/qmlimages/ui/compass_pointer.svg"
        fillMode: Image.PreserveAspectFit
        sourceSize.height: height
        anchors.top: compassBar.bottom
        anchors.topMargin: -height / 2
        anchors.horizontalCenter: parent.horizontalCenter
    }

    FlyViewMissionCompleteDialog {
        missionController: _missionController
        geoFenceController: _geoFenceController
        rallyPointController: _rallyPointController
    }

    Row {
        id: multiVehiclePanelSelector
        anchors.margins: _toolsMargin
        anchors.top: parent.top
        anchors.right: parent.right
        width: _rightPanelWidth
        spacing: ScreenTools.defaultFontPixelWidth
        visible: QGroundControl.multiVehicleManager.vehicles.count > 1
                 && QGroundControl.corePlugin.options.flyView.showMultiVehicleList

        property bool showSingleVehiclePanel: !visible
                                              || singleVehicleRadio.checked

        QGCMapPalette {
            id: mapPal
            lightColors: true
        }

        QGCRadioButton {
            id: singleVehicleRadio
            text: qsTr("Single")
            checked: true
            textColor: mapPal.text
        }

        QGCRadioButton {
            text: qsTr("Multi-Vehicle")
            textColor: mapPal.text
        }
    }

    MultiVehicleList {
        anchors.margins: _toolsMargin
        anchors.top: multiVehiclePanelSelector.bottom
        anchors.right: parent.right
        width: _rightPanelWidth
        height: parent.height - y - _toolsMargin
        visible: !multiVehiclePanelSelector.showSingleVehiclePanel
    }

    FlyViewInstrumentPanel {
        id: instrumentPanel
        anchors.margins: _toolsMargin
        anchors.top: multiVehiclePanelSelector.visible ? multiVehiclePanelSelector.bottom : parent.top
        anchors.right: parent.right
        width: _rightPanelWidth
        spacing: _toolsMargin
        visible: QGroundControl.corePlugin.options.flyView.showInstrumentPanel
                 && multiVehiclePanelSelector.showSingleVehiclePanel
        availableHeight: parent.height - y - _toolsMargin

        property real rightInset: visible ? parent.width - x : 0
    }

    PhotoVideoControl {
        id: photoVideoControl
        anchors.margins: _toolsMargin
        anchors.right: parent.right
        width: _rightPanelWidth
        state: _verticalCenter ? "verticalCenter" : "topAnchor"
        states: [
            State {
                name: "verticalCenter"
                AnchorChanges {
                    target: photoVideoControl
                    anchors.top: undefined
                    anchors.verticalCenter: _root.verticalCenter
                }
            },
            State {
                name: "topAnchor"
                AnchorChanges {
                    target: photoVideoControl
                    anchors.verticalCenter: undefined
                    anchors.top: instrumentPanel.bottom
                }
            }
        ]

        property bool _verticalCenter: !QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.rawValue
    }

    TelemetryValuesBar {
        id: telemetryPanel
        x: recalcXPosition()
        anchors.margins: _toolsMargin

        // States for custom layout support
        states: [
            State {
                name: "bottom"
                when: telemetryPanel.bottomMode

                AnchorChanges {
                    target: telemetryPanel
                    anchors.top: undefined
                    anchors.bottom: parent.bottom
                    anchors.right: undefined
                    anchors.verticalCenter: undefined
                }

                PropertyChanges {
                    target: telemetryPanel
                    x: recalcXPosition()
                }
            },

            State {
                name: "right-video"
                when: !telemetryPanel.bottomMode && photoVideoControl.visible

                AnchorChanges {
                    target: telemetryPanel
                    anchors.top: photoVideoControl.bottom
                    anchors.bottom: undefined
                    anchors.right: parent.right
                    anchors.verticalCenter: undefined
                }
            },

            State {
                name: "right-novideo"
                when: !telemetryPanel.bottomMode && !photoVideoControl.visible

                AnchorChanges {
                    target: telemetryPanel
                    anchors.top: undefined
                    anchors.bottom: undefined
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        ]

        function recalcXPosition() {
            // First try centered
            var halfRootWidth = _root.width / 2
            var halfPanelWidth = telemetryPanel.width / 2
            var leftX = (halfRootWidth - halfPanelWidth) - _toolsMargin
            var rightX = (halfRootWidth + halfPanelWidth) + _toolsMargin
            if (leftX >= parentToolInsets.leftEdgeBottomInset
                    || rightX <= parentToolInsets.rightEdgeBottomInset) {
                // It will fit in the horizontalCenter
                return halfRootWidth - halfPanelWidth
            } else {
                // Anchor to left edge
                return parentToolInsets.leftEdgeBottomInset + _toolsMargin
            }
        }
    }

    //-- Virtual Joystick
    Loader {
        id: virtualJoystickMultiTouch
        z: QGroundControl.zOrderTopMost + 1
        width: parent.width - (_pipOverlay.width / 2)
        height: Math.min(parent.height * 0.25,
                         ScreenTools.defaultFontPixelWidth * 16)
        visible: _virtualJoystickEnabled
                 && !QGroundControl.videoManager.fullScreen
                 && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parentToolInsets.leftEdgeBottomInset
                              + ScreenTools.defaultFontPixelHeight * 2
        anchors.horizontalCenter: parent.horizontalCenter
        source: "qrc:/qml/VirtualJoystick.qml"
        active: _virtualJoystickEnabled
                && !(_activeVehicle ? _activeVehicle.usingHighLatencyLink : false)

        property bool autoCenterThrottle: QGroundControl.settingsManager.appSettings.virtualJoystickAutoCenterThrottle.rawValue

        property bool _virtualJoystickEnabled: QGroundControl.settingsManager.appSettings.virtualJoystick.rawValue
    }

    FlyViewToolStrip {
        id: toolStrip
        anchors.leftMargin: _toolsMargin + parentToolInsets.leftEdgeCenterInset
        anchors.topMargin: _toolsMargin + parentToolInsets.topEdgeLeftInset
        anchors.left: parent.left
        anchors.top: parent.top
        z: QGroundControl.zOrderWidgets
        maxHeight: parent.height - y - parentToolInsets.bottomEdgeLeftInset - _toolsMargin
        visible: !QGroundControl.videoManager.fullScreen

        onDisplayPreFlightChecklist: mainWindow.showPopupDialogFromComponent(
                                         preFlightChecklistPopup)

        property real leftInset: x + width
    }

    FlyViewAirspaceIndicator {
        anchors.top: parent.top
        anchors.topMargin: ScreenTools.defaultFontPixelHeight * 0.25
        anchors.horizontalCenter: parent.horizontalCenter
        z: QGroundControl.zOrderWidgets
        show: mapControl.pipState.state !== mapControl.pipState.pipState
    }

    VehicleWarnings {
        anchors.centerIn: parent
        z: QGroundControl.zOrderTopMost
    }

    MapScale {
        id: mapScale
        anchors.margins: _toolsMargin
        anchors.left: toolStrip.right
        anchors.top: parent.top
        mapControl: _mapControl
        buttonsOnLeft: false
        visible: !ScreenTools.isTinyScreen
                 && QGroundControl.corePlugin.options.flyView.showMapScale
                 && mapControl.pipState.state === mapControl.pipState.fullState

        property real centerInset: visible ? parent.height - y : 0
    }

    Component {
        id: preFlightChecklistPopup
        FlyViewPreFlightChecklistPopup {}
    }
}
