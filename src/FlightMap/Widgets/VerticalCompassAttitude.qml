/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.FactSystem
import QGroundControl.FlightMap
import QGroundControl.Palette

Rectangle {
    width:  ScreenTools.defaultFontPixelHeight * 10
    height: _outerRadius * 4
    radius: _outerRadius
    color:  QGroundControl.globalPalette.window

    property real extraInset:           0
    property real extraValuesWidth:     _outerRadius

    property real _outerMargin: (width * 0.05) / 2
    property real _outerRadius: width / 2
    property real _innerRadius: _outerRadius - _outerMargin

    // Prevent all clicks from going through to lower layers
    DeadMouseArea {
        anchors.fill: parent
    }

    QGCAttitudeWidget {
        id:                         attitude
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.topMargin:          _outerMargin
        anchors.top:                parent.top
        size:                       _innerRadius * 2
        vehicle:                    globals.activeVehicle
    }

    QGCCompassWidget {
        id:                         compass
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.topMargin:          _outerMargin * 2
        anchors.top:                attitude.bottom
        size:                       _innerRadius * 2
        vehicle:                    globals.activeVehicle
    }
}
