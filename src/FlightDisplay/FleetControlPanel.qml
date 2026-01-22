/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Fleet Control Panel
/// Displays vehicle status grid on left and quick action buttons on right
Item {
    id: root

    // External dependencies (provided by FlyView)
    property var guidedActionsController
    property var missionController

    // Internal access
    property var _multiVehicleManager: QGroundControl.multiVehicleManager
    property var _activeVehicle: _multiVehicleManager.activeVehicle

    // Let parent control size (FlyView overlay)
    implicitWidth:  ScreenTools.defaultFontPixelWidth * 100
    implicitHeight: ScreenTools.defaultFontPixelHeight * 50

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    RowLayout {
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelWidth
        spacing: ScreenTools.defaultFontPixelWidth

        // =========================================================
        // LEFT PANEL — VEHICLE STATUS GRID
        // =========================================================
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: root.width * 0.4
            color: qgcPal.window
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            border.color: qgcPal.text
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing: ScreenTools.defaultFontPixelHeight * 0.6

                QGCLabel {
                    text: qsTr("FLEET STATUS")
                    font.bold: true
                    font.pointSize: ScreenTools.largeFontPointSize
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: qgcPal.text
                }

                GridView {
                    id: vehicleGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    model: _multiVehicleManager.vehicles

                    // Adaptive QGC-style sizing
                    cellWidth: Math.max(
                        ScreenTools.defaultFontPixelWidth * 22,
                        width / Math.max(1,
                            Math.floor(width / (ScreenTools.defaultFontPixelWidth * 24)))
                    )

                    cellHeight: ScreenTools.defaultFontPixelHeight * 20

                    delegate: VehicleStatusCard {
                        vehicle: object
                        width: vehicleGrid.cellWidth
                        height: vehicleGrid.cellHeight
                    }

                    QGCLabel {
                        anchors.centerIn: parent
                        visible: vehicleGrid.count === 0
                        text: qsTr("No vehicles connected")
                        font.pointSize: ScreenTools.largeFontPointSize
                        color: qgcPal.text
                    }
                }
            }
        }

        // =========================================================
        // RIGHT PANEL — QUICK ACTIONS
        // =========================================================
        Rectangle {
            Layout.fillHeight: true
            Layout.fillWidth: true
            color: qgcPal.window
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            border.color: qgcPal.text
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing: ScreenTools.defaultFontPixelHeight * 0.8

                QGCLabel {
                    text: qsTr("QUICK ACTIONS")
                    font.bold: true
                    font.pointSize: ScreenTools.largeFontPointSize
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: qgcPal.text
                }

                // -----------------------------------------------------
                // Action Mode Selector
                // -----------------------------------------------------
                ButtonGroup { id: actionModeGroup }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        text: qsTr("Active Vehicle")
                        checkable: true
                        checked: true
                        ButtonGroup.group: actionModeGroup
                        onClicked: actionStack.currentIndex = 0
                    }

                    QGCButton {
                        text: qsTr("All Vehicles")
                        checkable: true
                        ButtonGroup.group: actionModeGroup
                        onClicked: actionStack.currentIndex = 1
                    }
                }

                // -----------------------------------------------------
                // Action Panels
                // -----------------------------------------------------
                StackLayout {
                    id: actionStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: 0

                    // Active Vehicle Actions
                    QuickActionsPanel {
                        isActiveVehicleOnly: true
                        guidedController: root.guidedActionsController
                    }

                    // All Vehicles Actions
                    QuickActionsPanel {
                        isActiveVehicleOnly: false
                        guidedController: root.guidedActionsController
                    }
                }
            }
        }
    }
}
