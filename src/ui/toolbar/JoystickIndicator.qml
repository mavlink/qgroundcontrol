/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

// Joystick Indicator
Item {
    width:          joystickRow.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    visible:        activeVehicle ? activeVehicle.sub : false


    Component {
        id: joystickInfo

        Rectangle {
            width:  joystickCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: joystickCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 joystickCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(joystickGrid.width, joystickLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             joystickLabel
                    text:           qsTr("Joystick Status")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 joystickGrid
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("Connected:") }
                    QGCLabel {
                        text:  joystickManager.activeJoystick ? "Yes" : "No"
                        color: joystickManager.activeJoystick ? qgcPal.buttonText : "red"
                    }
                    QGCLabel { text: qsTr("Enabled:") }
                    QGCLabel {
                        text:  activeVehicle && activeVehicle.joystickEnabled ? "Yes" : "No"
                        color: activeVehicle && activeVehicle.joystickEnabled ? qgcPal.buttonText : "red"
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
            color:              activeVehicle && activeVehicle.joystickEnabled && joystickManager.activeJoystick ? qgcPal.buttonText : "red"
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked: {
            var centerX = mapToGlobal(x + (width / 2), 0).x
            mainWindow.showPopUp(joystickInfo, centerX)
        }
    }
}
