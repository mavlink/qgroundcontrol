/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL & VOLADOR AEROSPACE PROJECT
 *
 * Volador Ground Control Station - Modern Vehicle Setup & Configuration
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.AutoPilotPlugin
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import VoladorTheme 1.0
import VoladorComponents 1.0

Rectangle {
    id: setupView
    color: ThemeController.background
    z: QGroundControl.zOrderTopMost

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ButtonGroup { id: setupButtonGroup }

    readonly property real      _defaultTextHeight: ScreenTools.defaultFontPixelHeight
    readonly property real      _defaultTextWidth:  ScreenTools.defaultFontPixelWidth
    readonly property real      _horizontalMargin:  _defaultTextWidth / 2
    readonly property real      _verticalMargin:    _defaultTextHeight / 2
    readonly property real      _buttonWidth:       _defaultTextWidth * 18
    readonly property string    _armedVehicleText:  qsTr("This operation cannot be performed while the vehicle is armed.")

    property bool   _vehicleArmed:                  QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.armed : false
    property string _messagePanelText:              qsTr("missing message panel text")
    property bool   _fullParameterVehicleAvailable: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable && !QGroundControl.multiVehicleManager.activeVehicle.parameterManager.missingParameters
    property var    _corePlugin:                    QGroundControl.corePlugin

    function showSummaryPanel() {
        if (mainWindow.allowViewSwitch()) {
            _showSummaryPanel()
        }
    }

    function _showSummaryPanel() {
        if (_fullParameterVehicleAvailable) {
            if (QGroundControl.multiVehicleManager.activeVehicle.autopilotPlugin.vehicleComponents.length === 0) {
                panelLoader.setSourceComponent(noComponentsVehicleSummaryComponent)
            } else {
                panelLoader.setSource("VehicleSummary.qml")
            }
        } else if (QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable) {
            panelLoader.setSourceComponent(missingParametersVehicleSummaryComponent)
        } else {
            panelLoader.setSourceComponent(disconnectedVehicleSummaryComponent)
        }
        summaryButton.checked = true
    }

    function showPanel(button, qmlSource) {
        if (mainWindow.allowViewSwitch()) {
            button.checked = true
            panelLoader.setSource(qmlSource)
        }
    }

    function showVehicleComponentPanel(vehicleComponent) {
        if (mainWindow.allowViewSwitch()) {
            var autopilotPlugin = QGroundControl.multiVehicleManager.activeVehicle.autopilotPlugin
            var prereq = autopilotPlugin.prerequisiteSetup(vehicleComponent)
            if (prereq !== "") {
                _messagePanelText = qsTr("%1 setup must be completed prior to %2 setup.").arg(prereq).arg(vehicleComponent.name)
                panelLoader.setSourceComponent(messagePanelComponent)
            } else {
                panelLoader.setSource(vehicleComponent.setupSource, vehicleComponent)
                for (var i = 0; i < componentRepeater.count; i++) {
                    var obj = componentRepeater.itemAt(i);
                    if (obj.text === vehicleComponent.name) {
                        obj.checked = true
                        break;
                    }
                }
            }
        }
    }

    function showParametersPanel() {
        if (mainWindow.allowViewSwitch()) {
            parametersButton.checked = true
            panelLoader.setSource("SetupParameterEditor.qml")
        }
    }

    Component.onCompleted: _showSummaryPanel()

    Connections {
        target: QGroundControl.multiVehicleManager
        onActiveVehicleChanged: showSummaryPanel()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // LEFT CATEGORY NAVIGATION RAIL (Width 260)
        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 260
            color: ThemeController.sidebar
            border.color: ThemeController.border
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // Header Title & Search
                ColumnLayout {
                    spacing: 8
                    Text {
                        text: "VEHICLE SETUP"
                        font.family: "Inter"
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        color: ThemeController.accent
                    }

                    SearchField {
                        Layout.fillWidth: true
                        placeholderText: "Search setup..."
                    }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: ThemeController.border }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 6

                        // Summary Button
                        SubMenuButton {
                            id: summaryButton
                            Layout.fillWidth: true
                            text: qsTr("Summary")
                            imageResource: "/qmlimages/VehicleSummaryIcon.svg"
                            buttonGroup: setupButtonGroup
                            onClicked: showSummaryPanel()
                        }

                        // Firmware Upgrade
                        SubMenuButton {
                            id: firmwareButton
                            Layout.fillWidth: true
                            text: qsTr("Firmware")
                            imageResource: "/qmlimages/FirmwareUpgradeIcon.svg"
                            buttonGroup: setupButtonGroup
                            onClicked: showPanel(firmwareButton, "FirmwareUpgrade.qml")
                        }

                        // Component Repeater
                        Repeater {
                            id: componentRepeater
                            model: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle.autopilotPlugin.vehicleComponents : 0

                            SubMenuButton {
                                id: compButton
                                Layout.fillWidth: true
                                text: modelData.name
                                imageResource: modelData.iconResource
                                buttonGroup: setupButtonGroup
                                onClicked: showVehicleComponentPanel(modelData)
                            }
                        }

                        // Parameters Button
                        SubMenuButton {
                            id: parametersButton
                            Layout.fillWidth: true
                            text: qsTr("Parameters")
                            imageResource: "/qmlimages/ParametersIcon.svg"
                            buttonGroup: setupButtonGroup
                            onClicked: showParametersPanel()
                        }
                    }
                }
            }
        }

        // RIGHT CONTENT PANEL CONTAINER
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: ThemeController.background

            Loader {
                id: panelLoader
                anchors.fill: parent
                anchors.margins: 16
            }
        }
    }

    Component {
        id: disconnectedVehicleSummaryComponent
        Card {
            anchors.centerIn: parent
            implicitWidth: 400; implicitHeight: 200
            Column {
                anchors.centerIn: parent; spacing: 12
                Text { text: "⚠️ VEHICLE DISCONNECTED"; font.family: "Inter"; font.pixelSize: 18; font.weight: Font.Bold; color: ThemeController.warning }
                Text { text: "Connect vehicle via telemetry link to access configuration."; font.family: "Inter"; font.pixelSize: 13; color: ThemeController.textSecondary }
            }
        }
    }

    Component {
        id: missingParametersVehicleSummaryComponent
        Card {
            anchors.centerIn: parent
            implicitWidth: 400; implicitHeight: 200
            Column {
                anchors.centerIn: parent; spacing: 12
                Text { text: "⏳ PARAMETERS LOADING..."; font.family: "Inter"; font.pixelSize: 18; font.weight: Font.Bold; color: ThemeController.accent }
                Text { text: "Fetching parameter table from flight controller..."; font.family: "Inter"; font.pixelSize: 13; color: ThemeController.textSecondary }
            }
        }
    }

    Component {
        id: noComponentsVehicleSummaryComponent
        Card {
            anchors.centerIn: parent
            implicitWidth: 400; implicitHeight: 200
            Column {
                anchors.centerIn: parent; spacing: 12
                Text { text: "ℹ️ NO COMPONENTS DETECTED"; font.family: "Inter"; font.pixelSize: 18; font.weight: Font.Bold; color: ThemeController.textPrimary }
            }
        }
    }

    Component {
        id: messagePanelComponent
        Card {
            anchors.centerIn: parent
            implicitWidth: 440; implicitHeight: 220
            Column {
                anchors.centerIn: parent; spacing: 12
                Text { text: _messagePanelText; font.family: "Inter"; font.pixelSize: 14; color: ThemeController.textPrimary; wrapMode: Text.WordWrap }
            }
        }
    }
}
