/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                             button
    height:                         _infoCol.height * 1.25
    autoExclusive:                  true

    property var vehicle:           null

    function getBatteryColor() {
        if(vehicle) {
            if(vehicle.battery.percentRemaining.value > 75) {
                return qgcPal.colorGreen
            }
            if(vehicle.battery.percentRemaining.value > 50) {
                return qgcPal.colorOrange
            }
            if(vehicle.battery.percentRemaining.value > 0.1) {
                return qgcPal.colorRed
            }
        }
        return qgcPal.colorGrey
    }

    function getBatteryPercentage() {
        if(vehicle) {
            return vehicle.battery.percentRemaining.value / 100.0
        }
        return 1
    }

    background: Rectangle {
        anchors.fill:               parent
        color:                      button.checked ? qgcPal.buttonHighlight : qgcPal.button
        radius:                     ScreenTools.defaultFontPixelWidth * 0.5
    }

    contentItem: Row {
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.margins:            ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter:     button.verticalCenter
//        QGCColoredImage {
//            id:                     _icon
//            height:                 ScreenTools.defaultFontPixelHeight * 1.5
//            width:                  height
//            sourceSize.height:      parent.height
//            fillMode:               Image.PreserveAspectFit
//            color:                  button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
//            source:                 "/qmlimages/PaperPlane.svg"
//            anchors.verticalCenter: parent.verticalCenter
//        }
        Column {
            id:                     _infoCol
            spacing:                ScreenTools.defaultFontPixelHeight * 0.25
            QGCLabel {
                text:               qsTr("Vehicle ") + (vehicle ? vehicle.id : qsTr("None"))
                font.family:        ScreenTools.demiboldFontFamily
                color:              button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            }
            Row {
                spacing:            ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:           vehicle ? vehicle.flightMode : qsTr("None")
                    color:          button.checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
                }
                Rectangle {
                    height:         ScreenTools.defaultFontPixelHeight * 0.5
                    width:          ScreenTools.defaultFontPixelWidth  * 3
                    color:          Qt.rgba(0,0,0,0)
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        height:     parent.height
                        width:      parent.width * getBatteryPercentage()
                        color:      getBatteryColor()
                        anchors.right: parent.right
                    }
                }
            }
        }
    }

}
