/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

QGROUNDCONTROL is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QGROUNDCONTROL is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/// @file
///     @brief Setup View
///     @author Don Gagne <don@thegagnes.com>

import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl                       1.0
import QGroundControl.AutoPilotPlugin       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

Rectangle {
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup { id: setupButtonGroup }

    readonly property real      _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real      _defaultTextWidth:  ScreenTools.defaultFontPixelWidth
    readonly property real      _margin:            Math.round(_defaultTextHeight / 2)
    readonly property real      _buttonWidth:       Math.round(_defaultTextWidth * 18)
    readonly property string    _armedVehicleText:  qsTr("This operation cannot be performed while vehicle is armed.")

    property string _messagePanelText:              "missing message panel text"
    property bool   _fullParameterVehicleAvailable: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable && !QGroundControl.multiVehicleManager.activeVehicle.missingParameters

    function showSummaryPanel()
    {
        if (_fullParameterVehicleAvailable) {
            if (QGroundControl.multiVehicleManager.activeVehicle.autopilot.vehicleComponents.length == 0) {
                panelLoader.sourceComponent = noComponentsVehicleSummaryComponent
            } else {
                panelLoader.source = "VehicleSummary.qml";
            }
        } else if (QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable) {
            panelLoader.sourceComponent = missingParametersVehicleSummaryComponent
        } else {
            panelLoader.sourceComponent = disconnectedVehicleSummaryComponent
        }
    }

    function showFirmwarePanel()
    {
        if (!ScreenTools.isMobile) {
            if (QGroundControl.multiVehicleManager.activeVehicleAvailable && QGroundControl.multiVehicleManager.activeVehicle.armed) {
                _messagePanelText = _armedVehicleText
                panelLoader.sourceComponent = messagePanelComponent
            } else {
                panelLoader.source = "FirmwareUpgrade.qml";
            }
        }
    }

    function showJoystickPanel()
    {
        if (QGroundControl.multiVehicleManager.activeVehicleAvailable && QGroundControl.multiVehicleManager.activeVehicle.armed) {
            _messagePanelText = _armedVehicleText
            panelLoader.sourceComponent = messagePanelComponent
        } else {
            panelLoader.source = "JoystickConfig.qml";
        }
    }

    function showParametersPanel()
    {
        panelLoader.source = "SetupParameterEditor.qml";
    }

    function showPX4FlowPanel()
    {
        panelLoader.source = "PX4FlowSensor.qml";
    }

    function showVehicleComponentPanel(vehicleComponent)
    {
        if (QGroundControl.multiVehicleManager.activeVehicle.armed && !vehicleComponent.allowSetupWhileArmed) {
            _messagePanelText = _armedVehicleText
            panelLoader.sourceComponent = messagePanelComponent
        } else {
            if (vehicleComponent.prerequisiteSetup != "") {
                _messagePanelText = vehicleComponent.prerequisiteSetup + " setup must be completed prior to " + vehicleComponent.name + " setup."
                panelLoader.sourceComponent = messagePanelComponent
            } else {
                panelLoader.source = vehicleComponent.setupSource
            }
        }
    }

    Component.onCompleted: showSummaryPanel()

    Connections {
        target: QGroundControl.multiVehicleManager

        onParameterReadyVehicleAvailableChanged: {
            if (parameterReadyVehicleAvailable || summaryButton.checked || setupButtonGroup.current != firmwareButton) {
                // Show/Reload the Summary panel when:
                //      A new vehicle shows up
                //      The summary panel is already showing and the active vehicle goes away
                //      The active vehicle goes away and we are not on the Firmware panel.
                summaryButton.checked = true
                showSummaryPanel()
            }
        }
    }

    Component {
        id: noComponentsVehicleSummaryComponent

        Rectangle {
            color: qgcPal.windowShade

            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                text:                   "QGroundControl does not currently support setup of your vehicle type. " +
                                        "If your vehicle is already configured you can still Fly."

                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }

    Component {
        id: disconnectedVehicleSummaryComponent

        Rectangle {
            color: qgcPal.windowShade

            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pixelSize:         ScreenTools.largeFontPixelSize
                text:                   "Connect vehicle to your device and QGroundControl will automatically detect it." +
                                        (ScreenTools.isMobile ? "" : " Click Firmware on the left to upgrade your vehicle.")

                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }
    Component {
        id: missingParametersVehicleSummaryComponent

        Rectangle {
            color: qgcPal.windowShade

            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                text:                   "You are currently connected to a vehicle, but that vehicle did not return back the full parameter list. " +
                                        "Because of this the full set of vehicle setup options are not available."

                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }

    Component {
        id: messagePanelComponent

        Item {
            QGCLabel {
                anchors.margins:        _defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                text:                   _messagePanelText
            }
        }
    }

    QGCFlickable {
        id:                 buttonScroll
        width:              buttonColumn.width
        anchors.topMargin:  _defaultTextHeight / 2
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        contentHeight:      buttonColumn.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:         buttonColumn
            width:      _maxButtonWidth
            spacing:    _defaultTextHeight / 2

            property real _maxButtonWidth: 0

            Component.onCompleted: reflowWidths()

            Connections {
                target: componentRepeater

                onModelChanged: buttonColumn.reflowWidths()
            }

            function reflowWidths() {
                for (var i=0; i<children.length; i++) {
                    _maxButtonWidth = Math.max(_maxButtonWidth, children[i].width)
                }
                for (var i=0; i<children.length; i++) {
                    children[i].width = _maxButtonWidth
                }
            }

            SubMenuButton {
                id:             summaryButton
                imageResource: "/qmlimages/VehicleSummaryIcon.png"
                setupIndicator: false
                checked:        true
                exclusiveGroup: setupButtonGroup
                text:           "Summary"

                onClicked: showSummaryPanel()
            }

            SubMenuButton {
                id:             firmwareButton
                imageResource:  "/qmlimages/FirmwareUpgradeIcon.png"
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                visible:        !ScreenTools.isMobile
                text:           "Firmware"

                onClicked: showFirmwarePanel()
            }

            SubMenuButton {
                id:             px4FlowButton
                exclusiveGroup: setupButtonGroup
                visible:        QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.genericFirmware : false
                setupIndicator: false
                text:           "PX4Flow"
                onClicked:      showPX4FlowPanel()
            }

            SubMenuButton {
                id:             joystickButton
                setupIndicator: true
                setupComplete:  joystickManager.activeJoystick ? joystickManager.activeJoystick.calibrated : false
                exclusiveGroup: setupButtonGroup
                visible:        _fullParameterVehicleAvailable && joystickManager.joysticks.length != 0
                text:           "Joystick"

                onClicked: showJoystickPanel()
            }

            Repeater {
                id:     componentRepeater
                model:  _fullParameterVehicleAvailable ? QGroundControl.multiVehicleManager.activeVehicle.autopilot.vehicleComponents : 0

                SubMenuButton {
                    imageResource:  modelData.iconResource
                    setupIndicator: modelData.requiresSetup
                    setupComplete:  modelData.setupComplete
                    exclusiveGroup: setupButtonGroup
                    text:           modelData.name
                    visible:        modelData.setupSource.toString() != ""


                    onClicked: showVehicleComponentPanel(modelData)
                }
            }

            SubMenuButton {
                setupIndicator: false
                exclusiveGroup: setupButtonGroup
                visible:        QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable
                text:           "Parameters"

                onClicked: showParametersPanel()
            }

        }
    }

    Loader {
        id:                     panelLoader
        anchors.topMargin:      _margin
        anchors.bottomMargin:   _margin
        anchors.leftMargin:     _defaultTextWidth
        anchors.rightMargin:    _defaultTextWidth
        anchors.left:           buttonScroll.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
    }
}
