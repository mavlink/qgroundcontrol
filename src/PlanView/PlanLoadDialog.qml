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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.Controllers
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.ScreenTools

QGCPopupDialog {
    id:         root
    title:      qsTr("Load Plan")
    buttons:    Dialog.Cancel

    property var planMasterController

    property real _margin:              ScreenTools.defaultFontPixelWidth
    property real _sectionLeftMargin:   ScreenTools.defaultFontPixelWidth * 2
    property var  _appSettings:         QGroundControl.settingsManager.appSettings

    onRejected: ApplicationWindow.window.popView()

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight / 2

        QGCLabel {
            Layout.leftMargin:  _sectionLeftMargin
            text:               qsTr("No saved plans found.")
            visible:            fileRepeater.model.length === 0
            enabled:            false
        }

        Flow {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            Repeater {
                id:                 fileRepeater
                Layout.leftMargin:  _sectionLeftMargin
                model:              planMasterController.availablePlanNames()

                QGCButton {
                    text: modelData
                    onClicked: {
                        planMasterController.loadPlan(modelData)
                        planMasterController.fitViewportToItems()
                        planMasterController.missionController.setCurrentPlanViewSeqNum(0, true)
                        root.close()
                    }
                }
            }
        }

        QGCLabel {
            text: qsTr("Import other file formats or from other locations")
        }

        QGCButton {
            text:       qsTr("Import")
            onClicked:  importDialogComponent.createObject(mainWindow).openForLoad()
        }
    }

    Component {
        id: importDialogComponent

        QGCFileDialog {
            id:             importDialog
            folder:         _appSettings.planSavePath
            title:          qsTr("Import")
            nameFilters:    planMasterController.loadNameFilters

            onAcceptedForLoad: (file) => {
                root.close()
                planMasterController.import(file)
            }
        }
    }
}
