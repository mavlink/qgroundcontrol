/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQml.Models                 2.1

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Vehicle       1.0

Rectangle {
    width:      mainColumn.width  + ScreenTools.defaultFontPixelWidth * 3
    height:     Math.min(availableHeight - (_verticalMargin * 2), mainColumn.height + ScreenTools.defaultFontPixelHeight)
    color:      qgcPal.windowShade
    radius:     3

    property real _verticalMargin: ScreenTools.defaultFontPixelHeight / 2

    Loader {
        id:     modelContainer
        source: "/checklists/DefaultChecklist.qml"
    }

    property bool _passed:  false

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
        _passed = allPassed;
    }

    //-- Pick a checklist model that matches the current airframe type (if any)
    function _updateModel() {
        if(activeVehicle) {
            if(activeVehicle.multiRotor) {
                modelContainer.source = "/checklists/MultiRotorChecklist.qml"
            } else if(activeVehicle.vtol) {
                modelContainer.source = "/checklists/VTOLChecklist.qml"
            } else if(activeVehicle.rover) {
                modelContainer.source = "/checklists/RoverChecklist.qml"
            } else if(activeVehicle.sub) {
                modelContainer.source = "/checklists/SubChecklist.qml"
            } else if(activeVehicle.fixedWing) {
                modelContainer.source = "/checklists/FixedWingChecklist.qml"
            } else {
                modelContainer.source = "/checklists/DefaultChecklist.qml"
            }
            return
        }
        modelContainer.source = "/checklists/DefaultChecklist.qml"
    }

    Component.onCompleted: {
        _updateModel()
    }

    onVisibleChanged: {
        if(activeVehicle) {
            if(visible) {
                _updateModel()
            } else {
                if(modelContainer.item.model.isPassed()) {
                    activeVehicle.checkListState = Vehicle.CheckListPassed
                } else {
                    activeVehicle.checkListState = Vehicle.CheckListFailed
                }
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

    QGCFlickable {
        id:                     flickable
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.fill:           parent
        flickableDirection:     Flickable.VerticalFlick
        contentWidth:           mainColumn.width
        contentHeight:          mainColumn.height

        property real _horizontalMargin:    1.5 * ScreenTools.defaultFontPixelWidth
        property real _verticalMargin:      0.6 * ScreenTools.defaultFontPixelWidth

        Column {
            id:         mainColumn
            width:      40  * ScreenTools.defaultFontPixelWidth
            spacing:    0.8 * ScreenTools.defaultFontPixelWidth

            function groupPassedChanged(index, passed) {
                if (passed) {
                    delayedGroupPassed.index = index
                    delayedGroupPassed.restart()
                } else {
                    _handleGroupPassedChanged(index, passed)
                }
            }

            // Header/title of checklist
            Item {
                width:      parent.width
                height:     1.75 * ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    text:                   qsTr("Pre-Flight Checklist %1").arg(_passed ? qsTr("(passed)") : "")
                    anchors.left:           parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    font.pointSize:         ScreenTools.mediumFontPointSize
                }
                QGCButton {
                    width:                  1.2 * ScreenTools.defaultFontPixelHeight
                    height:                 1.2 * ScreenTools.defaultFontPixelHeight
                    anchors.right:          parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    tooltip:                qsTr("Reset the checklist (e.g. after a vehicle reboot)")

                    onClicked:              checkListRepeater.model.reset()

                    QGCColoredImage {
                        source:         "/qmlimages/MapSyncBlack.svg"
                        color:          qgcPal.buttonText
                        anchors.fill:   parent
                    }
                }
            }

            // All check list items
            Repeater {
                id:     checkListRepeater
                model:  modelContainer.item.model
            }
        }
    }
}
