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

import QGroundControl.AutoPilotPlugin       1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

Item {
    z: zOrder   // zOrder comes from the Loader in MainWindow.qml

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup { id: setupButtonGroup }

    readonly property real      _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real      _defaultTextWidth:  ScreenTools.defaultFontPixelWidth
    readonly property real      _margin:            _defaultTextHeight / 2
    readonly property real      _buttonWidth:       _defaultTextWidth * 15
    readonly property string    _armedVehicleText:  "This operation cannot be performed while vehicle is armed."

    property string _messagePanelText:              "missing message panel text"
    property bool   _fullParameterVehicleAvailable: multiVehicleManager.parameterReadyVehicleAvailable && !multiVehicleManager.activeVehicle.missingParameters

    function showSummaryPanel()
    {
        if (_fullParameterVehicleAvailable) {
            panelLoader.source = "VehicleSummary.qml";
        } else if (multiVehicleManager.parameterReadyVehicleAvailable) {
            panelLoader.sourceComponent = missingParametersVehicleSummaryComponent
        } else {
            panelLoader.sourceComponent = disconnectedVehicleSummaryComponent
        }
    }

    function showFirmwarePanel()
    {
        if (!ScreenTools.isMobile) {
            if (multiVehicleManager.activeVehicleAvailable && multiVehicleManager.activeVehicle.armed) {
                _messagePanelText = _armedVehicleText
                panelLoader.sourceComponent = messagePanelComponent
            } else {
                panelLoader.source = "FirmwareUpgrade.qml";
            }
        }
    }

    function showJoystickPanel()
    {
        if (multiVehicleManager.activeVehicleAvailable && multiVehicleManager.activeVehicle.armed) {
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

    function showVehicleComponentPanel(vehicleComponent)
    {
        if (multiVehicleManager.activeVehicle.armed) {
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
        target: multiVehicleManager

        onParameterReadyVehicleAvailableChanged: {
            summaryButton.checked = true
            showSummaryPanel()
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
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                text:                   "Welcome to QGroundControl. " +
                                        "QGroundControl supports any <font color=\"orange\"><a href=\"http://www.qgroundcontrol.org/mavlink/start\">mavlink</a></font> enabled vehicle. " +
                                        "If you are using the <font color=\"orange\"><a href=\"https://pixhawk.org/choice\">PX4 Flight Stack</a></font>, you also get full support for setting up and calibrating your vehicle. "+
                                        "Otherwise you will only get support for flying a vehicle which has been setup and calibrated using other means. " +
                                        "Use the Connect button above to connect to your vehicle."

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

    Rectangle {
        //anchors.margins:    _defaultTextHeight * 2
        anchors.fill:       parent
        color:              qgcPal.window
        opacity:            0.8

        QGCLabel {
            id:                     title
            anchors.topMargin:      _margin
            anchors.top:            parent.top
            anchors.left:           parent.left
            anchors.right:          parent.right
            horizontalAlignment:    Text.AlignHCenter
            font.pixelSize:         ScreenTools.largeFontPixelSize
            text:                   "Vehicle Setup"
        }

        Rectangle {
            anchors.topMargin:  _margin
            anchors.top:        title.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            color:              qgcPal.windowShade

            Flickable {
                id:                 buttonFlickable
                width:              _buttonWidth
                height:             parent.height
                contentWidth:       _buttonWidth
                contentHeight:      buttonColumn.height
                flickableDirection: Flickable.VerticalFlick

                Column {
                    id:         buttonColumn
                    width:      _buttonWidth
                    spacing:    _defaultTextHeight / 2

                    SubMenuButton {
                        id:             summaryButton
                        width:          _buttonWidth
                        imageResource: "/qmlimages/VehicleSummaryIcon.png"
                        setupIndicator: false
                        exclusiveGroup: setupButtonGroup
                        text:           "SUMMARY"

                        onClicked: showSummaryPanel()
                    }

                    SubMenuButton {
                        id:             firmwareButton
                        width:          _buttonWidth
                        imageResource:  "/qmlimages/FirmwareUpgradeIcon.png"
                        setupIndicator: false
                        exclusiveGroup: setupButtonGroup
                        visible:        !ScreenTools.isMobile
                        text:           "FIRMWARE"

                        onClicked: showFirmwarePanel()
                    }

                    SubMenuButton {
                        id:             joystickButton
                        width:          _buttonWidth
                        setupIndicator: true
                        setupComplete:  joystickManager.activeJoystick ? joystickManager.activeJoystick.calibrated : false
                        exclusiveGroup: setupButtonGroup
                        visible:        _fullParameterVehicleAvailable && joystickManager.joysticks.length != 0
                        text:           "JOYSTICK"

                        onClicked: showJoystickPanel()
                    }

                    Repeater {
                        model: _fullParameterVehicleAvailable ? multiVehicleManager.activeVehicle.autopilot.vehicleComponents : 0

                        SubMenuButton {
                            width:          _buttonWidth
                            imageResource:  modelData.iconResource
                            setupIndicator: modelData.requiresSetup
                            setupComplete:  modelData.setupComplete
                            exclusiveGroup: setupButtonGroup
                            text:           modelData.name.toUpperCase()

                            onClicked: showVehicleComponentPanel(modelData)
                        }
                    }

                    SubMenuButton {
                        width:          _buttonWidth
                        setupIndicator: false
                        exclusiveGroup: setupButtonGroup
                        visible:        multiVehicleManager.parameterReadyVehicleAvailable
                        text:           "PARAMETERS"

                        onClicked: showParametersPanel()
                    }
                } // Column
            } // Flickable

            Loader {
                id:                     panelLoader
                anchors.leftMargin:     _defaultTextWidth
                anchors.rightMargin:    _defaultTextWidth
                anchors.left:           buttonFlickable.right
                anchors.right:          parent.right
                anchors.top:            parent.top
                anchors.bottom:         parent.bottom
            }
        }
    }
}
