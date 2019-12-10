/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQml.Models     2.1

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0

Rectangle {
    width:      mainColumn.width  + ScreenTools.defaultFontPixelWidth * 3
    height:     mainColumn.height + ScreenTools.defaultFontPixelHeight
    color:      qgcPal.windowShade
    radius:     3

    PreFlightCheckModel {
        id:     listModel
        PreFlightCheckGroup {
            name: qsTr("Initial checks")

            // Standard check list items (group 0) - Available from the start
            PreFlightCheckButton {
                name:           qsTr("Hardware")
                manualText:     qsTr("Props mounted? Wings secured? Tail secured?")
            }

            PreFlightBatteryCheck {
                failurePercent:                 40
                allowFailurePercentOverride:    false
            }

            PreFlightSensorsHealthCheck {
            }

            PreFlightGPSCheck {
                failureSatCount:        9
                allowOverrideSatCount:  true
            }

            PreFlightRCCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Please arm the vehicle here")

            PreFlightCheckButton {
                name:            qsTr("Actuators")
                manualText:      qsTr("Move all control surfaces. Did they work properly?")
            }

            PreFlightCheckButton {
                name:            qsTr("Motors")
                manualText:      qsTr("Propellers free? Then throttle up gently. Working properly?")
            }

            PreFlightCheckButton {
                name:        qsTr("Mission")
                manualText:  qsTr("Please confirm mission is valid (waypoints valid, no terrain collision).")
            }

            PreFlightSoundCheck {
            }
        }

        PreFlightCheckGroup {
            name: qsTr("Last preparations before launch")

            // Check list item group 2 - Final checks before launch
            PreFlightCheckButton {
                name:        qsTr("Payload")
                manualText:  qsTr("Configured and started? Payload lid closed?")
            }

            PreFlightCheckButton {
                name:        "Wind & weather"
                manualText:  qsTr("OK for your platform? Lauching into the wind?")
            }

            PreFlightCheckButton {
                name:        qsTr("Flight area")
                manualText:  qsTr("Launch area and path free of obstacles/people?")
            }
        }
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
        _passed = passed
    }

    // We delay the updates when a group passes so the user can see all items green for a moment prior to hiding
    Timer {
        id:         delayedGroupPassed
        interval:   750

        property int index

        onTriggered: _handleGroupPassedChanged(index, true /* passed */)
    }

    Column {
        id:                     mainColumn
        width:                  40  * ScreenTools.defaultFontPixelWidth
        spacing:                0.8 * ScreenTools.defaultFontPixelWidth
        anchors.left:           parent.left
        anchors.top:            parent.top
        anchors.topMargin:      0.6 * ScreenTools.defaultFontPixelWidth
        anchors.leftMargin:     1.5 * ScreenTools.defaultFontPixelWidth

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

                onClicked:              model.reset()

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
            model:  listModel
        }
    }
}
