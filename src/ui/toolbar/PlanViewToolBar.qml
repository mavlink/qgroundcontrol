/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.platform

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Controllers

Rectangle {
    id:     _root
    width:  parent.width
    height: ScreenTools.toolbarHeight
    color:  qgcPal.toolbarBackground

    property var planMasterController
    property var planView

    property alias layerIndex: layerCombo.layerIndex

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property real   _margins:                   ScreenTools.defaultFontPixelWidth
    property var    _appSettings:                QGroundControl.settingsManager.appSettings

    function checkReadyForSave() {
        var unableToSave = qsTr("Unable to save")
        var readyForSaveState = planMasterController.readyForSaveState()
        if (readyForSaveState == VisualMissionItem.NotReadyForSaveData) {
            ApplicationWindow.window.showMessageDialog(unableToSave, qsTr("Plan has incomplete items. Complete all items and save again."))
            return false
        } else if (readyForSaveState == VisualMissionItem.NotReadyForSaveTerrain) {
            ApplicationWindow.window.showMessageDialog(unableToSave, qsTr("Plan is waiting on terrain data from server for correct altitude values."))
            return false
        }
        return true
    }

    RowLayout {
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _margins
        anchors.left:       parent.left
        spacing:            ScreenTools.defaultFontPixelWidth

        ColumnLayout {
            spacing: 0

            QGCLabel {
                Layout.alignment:   Qt.AlignLeft
                text:               planMasterController.planName
                font.bold:          true
            }
            QGCLabel {
                Layout.alignment:   Qt.AlignLeft
                text:               planMasterController.planType
                font.pointSize:     ScreenTools.smallFontPointSize
            }
        }

        QGCButton {
            text: qsTr("Save")

            onClicked: {
                if (!checkReadyForSave()) {
                    return
                }
                planMasterController.savePlan()
            }
        }

        QGCButton {
            text:       qsTr("Plan Settings")
            onClicked:  planView.showPlanSettingsDialog()
        }

        QGCButton {
            text:       qsTr("Tools")
            onClicked:  toolsMenu.open()

            Menu {
                id: toolsMenu

                MenuItem {
                    text: qsTr("Close Plan")

                    onTriggered: {
                        if (planMasterController.dirty) {
                            mainWindow.showMessageDialog(
                                qsTr("Close Plan"), 
                                qsTr("Plan has unsaved changes. Are you sure you want to close without saving?"), 
                                Dialog.Yes | Dialog.No, 
                                function accept() { 
                                    planMasterController.closePlan()
                                })
                        } else {
                            planMasterController.closePlan()
                        }
                    }
                }
                MenuItem {
                    text: qsTr("Delete Plan")

                    onTriggered: {
                        mainWindow.showMessageDialog(
                            qsTr("Delete Plan"), 
                            qsTr("Are you sure you want to delete the plan?"), 
                            Dialog.Yes | Dialog.No, 
                            function accept() { 
                                planMasterController.deletePlan()
                            })
                    }
                }
                MenuItem {
                    text: qsTr("Save to KML")

                    onTriggered: {
                        saveToKMLDialogComponent.createObject(ApplicationWindow.window).openForSave()
                    }
                }
            }
        }
    }

    RowLayout {
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.rightMargin:    _margins
        anchors.right:          parent.right
        spacing:                ScreenTools.defaultFontPixelWidth

        QGCComboBox {
            id:             layerCombo
            model:          [ qsTr("Mission"), qsTr("Fence"), qsTr("Rally") ]
            currentIndex:   0
            visible:        QGroundControl.corePlugin.options.enablePlanViewSelector

            property int layerIndex: Math.max(currentIndex, 0)
        }
    }

    Component {
        id: saveToKMLDialogComponent

        QGCFileDialog {
            id:             saveToKMLDialog
            folder:         _appSettings.planSavePath
            title:          qsTr("Save Plan to KML")
            defaultSuffix:  _appSettings.kmlFileExtension

            onAcceptedForSave: (file) => {
                planMasterController.saveToKML(file)
            }
        }
    }
}
