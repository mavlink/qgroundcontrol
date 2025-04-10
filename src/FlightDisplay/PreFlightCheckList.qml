/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQml.Models
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Vehicle

ColumnLayout {
    spacing: 0.8 * ScreenTools.defaultFontPixelWidth

    property real _verticalMargin: ScreenTools.defaultFontPixelHeight / 2

    Loader {
        id:     modelContainer
        source: "/checklists/DefaultChecklist.qml"
    }

    property bool allChecksPassed:  false
    property var  vehicleCopy:      globals.activeVehicle

    onVehicleCopyChanged: {
        if (checkListRepeater.model) {
            checkListRepeater.model.reset()
        }
    }

    onAllChecksPassedChanged: {
        if (allChecksPassed) {
            globals.activeVehicle.checkListState = Vehicle.CheckListPassed
        } else {
            globals.activeVehicle.checkListState = Vehicle.CheckListFailed
        }
    }

    function _handleGroupPassedChanged(index, passed) {
        if (passed) {
            // Collapse current group
            var group = checkListRepeater.itemAt(index)
            group._checked = false
            // Expand next group
            if (index + 1 < checkListRepeater.count) {
                group = checkListRepeater.itemAt(index + 1)
                group.enabled = true
                group._checked = true
            }
        }

        // Walk the list and check if any group is failing
        var allPassed = true
        for (var i=0; i < checkListRepeater.count; i++) {
            if (!checkListRepeater.itemAt(i).passed) {
                allPassed = false
                break
            }
        }
        
        if (allPassed) {
            globals.activeVehicle.checkListItem1 = false
            globals.activeVehicle.checkListItem2 = false
            globals.activeVehicle.checkListItem3 = false
            globals.activeVehicle.checkListItem4 = false
            globals.activeVehicle.checkListItem5 = false
            globals.activeVehicle.checkListItem6 = false
            globals.activeVehicle.checkListItem7 = false
            globals.activeVehicle.checkListItem8 = false
            globals.activeVehicle.checkListItem9 = false
            globals.activeVehicle.checkListItem10 = false
            globals.activeVehicle.checkListItem11 = false
            globals.activeVehicle.checkListItem12 = false
            globals.activeVehicle.checkListItem13 = false
            globals.activeVehicle.checkListItem14 = false
            globals.activeVehicle.checkListItem15 = false
            globals.activeVehicle.checkListItem16 = false
            globals.activeVehicle.checkListItem17 = false
            globals.activeVehicle.checkListItem18 = false
            globals.activeVehicle.checkListItem19 = false
            globals.activeVehicle.checkListItem20 = false
            globals.activeVehicle.checkListItem21 = false
        }
        allChecksPassed = allPassed;
    }

    //-- Pick a checklist model that matches the current airframe type (if any)
    function _updateModel() {
        var vehicle = globals.activeVehicle
        if (!vehicle) {
            vehicle = QGroundControl.multiVehicleManager.offlineEditingVehicle
        }

        if(vehicle.multiRotor) {
            modelContainer.source = "/checklists/MultiRotorChecklist.qml"
        } else if(vehicle.vtol) {
            modelContainer.source = "/checklists/VTOLChecklist.qml"
        } else if(vehicle.rover) {
            modelContainer.source = "/checklists/RoverChecklist.qml"
        } else if(vehicle.sub) {
            modelContainer.source = "/checklists/SubChecklist.qml"
        } else if(vehicle.fixedWing) {
            modelContainer.source = "/checklists/FixedWingChecklist.qml"
        } else {
            modelContainer.source = "/checklists/DefaultChecklist.qml"
        }
        return
    }

    Component.onCompleted: {
        _updateModel()
    }

    onVisibleChanged: {
        if(globals.activeVehicle) {
            if(visible) {
                _updateModel()
            }
        }
    }

    // We delay the updates when a group passes so the user can see all items green for a moment prior to hiding
    Timer {
        id:         delayedGroupPassed
        interval:   750

        property int index

        onTriggered: _handleGroupPassedChanged(index, true /* passed */)
    }

    function groupPassedChanged(index, passed) {
        if (passed) {
            delayedGroupPassed.index = index
            delayedGroupPassed.restart()
        } else {
            _handleGroupPassedChanged(index, passed)
        }
    }

    // Header/title of checklist
    RowLayout {
        Layout.fillWidth:   true
        height:             1.75 * ScreenTools.defaultFontPixelHeight
        spacing:            0

        QGCLabel {
            Layout.fillWidth:   true
            text:               allChecksPassed ? qsTr("(Passed)") : qsTr("In Progress")
            font.pointSize:     ScreenTools.mediumFontPointSize
        }
        QGCButton {
            width:              1.2 * ScreenTools.defaultFontPixelHeight
            height:             1.2 * ScreenTools.defaultFontPixelHeight
            Layout.alignment:   Qt.AlignVCenter
            onClicked: {
                resetCheckListItem()
                checkListRepeater.model.reset()
            }

            QGCColoredImage {
                source:         "/qmlimages/MapSyncBlack.svg"
                color:          qgcPal.buttonText
                anchors.fill:   parent
            }

            function resetCheckListItem() {
                globals.activeVehicle.checkListItem1 = false
                globals.activeVehicle.checkListItem2 = false
                globals.activeVehicle.checkListItem3 = false
                globals.activeVehicle.checkListItem4 = false
                globals.activeVehicle.checkListItem5 = false
                globals.activeVehicle.checkListItem6 = false
                globals.activeVehicle.checkListItem7 = false
                globals.activeVehicle.checkListItem8 = false
                globals.activeVehicle.checkListItem9 = false
                globals.activeVehicle.checkListItem10 = false
                globals.activeVehicle.checkListItem11 = false
                globals.activeVehicle.checkListItem12 = false
                globals.activeVehicle.checkListItem13 = false
                globals.activeVehicle.checkListItem14 = false
                globals.activeVehicle.checkListItem15 = false
                globals.activeVehicle.checkListItem16 = false
                globals.activeVehicle.checkListItem17 = false
                globals.activeVehicle.checkListItem18 = false
                globals.activeVehicle.checkListItem19 = false
                globals.activeVehicle.checkListItem20 = false
                globals.activeVehicle.checkListItem21 = false
            }
        }
    }

    // All check list items
    Repeater {
        id:     checkListRepeater
        model:  modelContainer.item.model
    }
}
