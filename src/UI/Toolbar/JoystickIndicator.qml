/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette

// Joystick Indicator
Item {
    id:             control
    width:          joystickRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    visible:        globals.activeVehicle ? globals.activeVehicle.sub : false


    Component {
        id: joystickInfoPage

        ToolIndicatorPage {
            showExpand: false

            contentComponent: SettingsGroupLayout {
                heading: qsTr("Joystick Status")

                GridLayout {
                    columns: 2

                    QGCLabel { text: qsTr("Connected:") }
                    QGCLabel {
                        text:  joystickManager.activeJoystick ? qsTr("Yes") : qsTr("No")
                        color: joystickManager.activeJoystick ? qgcPal.buttonText : "red"
                    }
                    QGCLabel { text: qsTr("Enabled:") }
                    QGCLabel {
                        text:  globals.activeVehicle && globals.activeVehicle.joystickEnabled ? qsTr("Yes") : qsTr("No")
                        color: globals.activeVehicle && globals.activeVehicle.joystickEnabled ? qgcPal.buttonText : "red"
                    }
                }
            }
        }
    }

    Row {
        id:             joystickRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth

        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/qmlimages/Joystick.png"
            fillMode:           Image.PreserveAspectFit
            color: {
                if(globals.activeVehicle && joystickManager.activeJoystick) {
                    if(globals.activeVehicle.joystickEnabled) {
                        // Everything ready to use joystick
                        return qgcPal.buttonText
                    }
                    // Joystick is not enabled in the joystick configuration page
                    return "yellow"
                }
                // Joystick not available or there is no active vehicle
                return "red"
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(joystickInfoPage, control)
    }
}
