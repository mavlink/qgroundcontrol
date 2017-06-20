/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- GPS Indicator
Item {
    id:             satelitte
    width:          (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    Component {
        id: gpsInfo

        Rectangle {
            width:  gpsCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: gpsCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 gpsCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(gpsGrid.width, gpsLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             gpsLabel
                    text: (QGroundControl.gpsRtk.active.value) ?  qsTr("Survey-in Active") : qsTr("RTK Active")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 gpsGrid
                    visible:            (activeVehicle && activeVehicle.gps.count.value >= 0)
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    columns: 2

                    QGCLabel { text: qsTr("GPS connected:") }
                    QGCLabel { text: QGroundControl.gpsRtk.connected.value }
                    QGCLabel { text: qsTr("Survey-in Duration:") }
                    QGCLabel { text: QGroundControl.gpsRtk.currentDuration.value }
                    QGCLabel { text: qsTr("Survey-in Accuracy:") }
                    QGCLabel { text: QGroundControl.gpsRtk.currentAccuracy.value }
                    QGCLabel { text: qsTr("Number of Satellites:") }
                    QGCLabel { text: QGroundControl.gpsRtk.numSatellites.value }
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
            }
        }
    }

    QGCColoredImage {
        id:                 gpsIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/Gps.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            (activeVehicle && activeVehicle.gps.count.value >= 0) ? 1 : 0.5
        color:              qgcPal.buttonText
    }

    Column {
        id:                     gpsValuesColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
        anchors.left:           gpsIcon.right

        QGCLabel {
            anchors.horizontalCenter:   hdopValue.horizontalCenter
            visible:                    activeVehicle && !isNaN(activeVehicle.gps.hdop.value)
            color:                      qgcPal.buttonText
            text:                       activeVehicle ? activeVehicle.gps.count.valueString : ""
        }

        QGCLabel {
            id:         hdopValue
            visible:    activeVehicle && !isNaN(activeVehicle.gps.hdop.value)
            color:      qgcPal.buttonText
            text:       activeVehicle ? activeVehicle.gps.hdop.value.toFixed(1) : ""
        }
    }
    
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            var centerX = mapToItem(toolBar, x, y).x + (width / 2)
            mainWindow.showPopUp(gpsInfo, centerX)
        }
    }
}
