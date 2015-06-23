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

import QtQuick 2.3
import QtQuick.Controls 1.2

import QGroundControl.AutoPilotPlugin 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

Rectangle {
    id:     topLevel
    color:  palette.window

    QGCPalette { id: palette; colorGroupEnabled: true }

    ExclusiveGroup { id: setupButtonGroup }

    QGCLabel { id: _textMeasure; text: "X"; visible: false }

    readonly property real defaultTextHeight:   _textMeasure.contentHeight
    readonly property real defaultTextWidth:    _textMeasure.contentWidth
    readonly property real buttonWidth:         defaultTextWidth * 15

    property string messagePanelText:           "missing message panel text"
    readonly property string armedVehicleText:  "This operation cannot be performed while vehicle is armed."

    function showSummaryPanel()
    {
        if (controller.autopilot) {
            panelLoader.source = "VehicleSummary.qml";
        } else {
            panelLoader.sourceComponent = disconnectedVehicleSummaryComponent
        }
    }

    function showFirmwarePanel()
    {
        if (controller.showFirmware) {
            if (controller.autopilot && controller.autopilot.armed) {
                messagePanelText = armedVehicleText
                panelLoader.sourceComponent = messagePanelComponent
            } else {
                panelLoader.source = "FirmwareUpgrade.qml";
            }
        }
    }

    function showParametersPanel()
    {
        panelLoader.source = "SetupParameterEditor.qml";
    }

    function showVehicleComponentPanel(vehicleComponent)
    {
        if (controller.autopilot.armed) {
            messagePanelText = armedVehicleText
            panelLoader.sourceComponent = messagePanelComponent
        } else {
            if (vehicleComponent.prerequisiteSetup != "") {
                messagePanelText = vehicleComponent.prerequisiteSetup + " setup must be completed prior to " + vehicleComponent.name + " setup."
                panelLoader.sourceComponent = messagePanelComponent
            } else {
                panelLoader.source = vehicleComponent.setupSource
            }
        }
    }

    Connections {
        target: controller

        onAutopilotChanged: {
            summaryButton.checked = true
            showSummaryPanel()
        }
    }

    Component.onCompleted: showSummaryPanel()

    Component {
        id: disconnectedVehicleSummaryComponent

        Rectangle {
            color: palette.windowShade

            QGCLabel {
                anchors.margins:        defaultTextWidth * 2
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
        id: messagePanelComponent

        Item {
            QGCLabel {
                anchors.margins:        defaultTextWidth * 2
                anchors.fill:           parent
                verticalAlignment:      Text.AlignVCenter
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
                font.pixelSize:         ScreenTools.mediumFontPixelSize
                text:                   messagePanelText
            }
        }
    }

    Column {
        id:     buttonColumn
        width:  buttonWidth

        SubMenuButton {
            id:             summaryButton
            width:          buttonWidth
            imageResource: "/qmlimages/VehicleSummaryIcon.png"
            setupIndicator: false
            exclusiveGroup: setupButtonGroup
            text:           "SUMMARY"

            onClicked: showSummaryPanel()
        }

        SubMenuButton {
            id:             firmwareButton
            width:          buttonWidth
            imageResource:  "/qmlimages/FirmwareUpgradeIcon.png"
            setupIndicator: false
            exclusiveGroup: setupButtonGroup
            visible:        controller.showFirmware
            text:           "FIRMWARE"

            onClicked: showFirmwarePanel()
        }

        Repeater {
            model: controller.autopilot ? controller.autopilot.vehicleComponents : 0

            SubMenuButton {
                width:          buttonWidth
                imageResource:  modelData.iconResource
                setupComplete:  modelData.setupComplete
                exclusiveGroup: setupButtonGroup
                text:           modelData.name.toUpperCase()

                onClicked: showVehicleComponentPanel(modelData)
            }
        }

        SubMenuButton {
            width:          buttonWidth
            setupIndicator: false
            exclusiveGroup: setupButtonGroup
            visible:        controller.autopilot
            text:           "PARAMETERS"

            onClicked: showParametersPanel()
        }
    } // Column

    Loader {
        id:                     panelLoader
        anchors.leftMargin:     defaultTextWidth
        anchors.rightMargin:    defaultTextWidth
        anchors.left:           buttonColumn.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom

        property var autopilot: controller.autopilot
    }
}
