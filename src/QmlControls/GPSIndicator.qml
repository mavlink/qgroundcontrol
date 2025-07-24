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

import QGroundControl.ScreenTools

// Used as the base class control for nboth VehicleGPSIndicator and RTKGPSIndicator

Item {
    id:             control
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rtkConnected:  QGroundControl.gpsRtk.connected.value

    Row {
        id:             gpsIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Row {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        -ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                id:                     gpsLabel
                rotation:               90
                text:                   qsTr("RTK")
                font.pixelSize:         ScreenTools.defaultFontPixelHeight * 0.6//my add
                color:                  qgcPal.buttonText
                anchors.verticalCenter: parent.verticalCenter
                visible:                _rtkConnected
            }

            QGCColoredImage {
                id: gpsIcon
                width: 20
                height: 20
                anchors.verticalCenter: parent.verticalCenter
                source: "/qmlimages/Gps.svg"
                fillMode: Image.PreserveAspectFit
                sourceSize.width: 16
                sourceSize.height: 16
                opacity: (_activeVehicle && _activeVehicle.gps.count.value >= 0) ? 1 : 0.5
                color: qgcPal.buttonText
            }
        }

        Column {
            id:                     gpsValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value)
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   hdopValue.horizontalCenter
                color:              qgcPal.buttonText
                text:               _activeVehicle ? _activeVehicle.gps.count.valueString : ""
                font.pixelSize:             ScreenTools.defaultFontPixelHeight * 0.6//my add
            }

            QGCLabel {
                id:     hdopValue
                color:  qgcPal.buttonText
                text:   _activeVehicle ? _activeVehicle.gps.hdop.value.toFixed(1) : ""
                font.pixelSize:             ScreenTools.defaultFontPixelHeight * 0.6//my add
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage

        GPSIndicatorPage { }
    }
}
