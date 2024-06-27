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

//-------------------------------------------------------------------------
//-- GPS Indicator
Item {
    id:             control
    width:          (gpsValuesColumn.x + gpsValuesColumn.width) * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool showIndicator: true

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCColoredImage {
        id:                 gpsIcon
        width:              height
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        source:             "/qmlimages/Gps.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        opacity:            (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
        color:              (_activeVehicle && _activeVehicle.gps.systemErrors.value !== 0) ? qgcPal.colorRed : qgcPal.buttonText
    }

    Column {
        id:                     gpsValuesColumn
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
        anchors.left:           gpsIcon.right

        QGCLabel {
            anchors.horizontalCenter:   hdopValue.horizontalCenter
            visible:                    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
            color:                      qgcPal.buttonText
            text:                       _activeVehicle ? _activeVehicle.gps.count.valueString : ""
        }

        QGCLabel {
            id:         hdopValue
            visible:    _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
            color:      qgcPal.buttonText
            text:       _activeVehicle ? _activeVehicle.gps.hdop.value.toFixed(1) : ""
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage {

        }
    }
}
