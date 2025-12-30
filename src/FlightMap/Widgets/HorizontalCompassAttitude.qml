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



Rectangle {
    id:     control
    width:  Math.min(_defaultWidth, _maxWidth)
    height: _outerRadius * 2
    radius: _outerRadius
    color:  qgcPal.window

    property real extraInset:           0
    property real extraValuesWidth:     _outerRadius

    property real   _defaultWidth:      mainWindow.width * 0.2
    property real   _maxWidth:          ScreenTools.defaultFontPixelHeight * 15
    property real   _innerRadius:       (width - (_topBottomMargin * 3)) / 4
    property real   _outerRadius:       _innerRadius + _topBottomMargin
    property real   _spacing:           ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin:   (width * 0.05) / 2

    DeadMouseArea { anchors.fill: parent }

    QGCPalette { id: qgcPal }

    QGCAttitudeWidget {
        id:                     attitude
        anchors.leftMargin:     control._topBottomMargin
        anchors.left:           parent.left
        size:                   control._innerRadius * 2
        vehicle:                globals.activeVehicle
        anchors.verticalCenter: parent.verticalCenter
    }

    QGCCompassWidget {
        id:                     compass
        anchors.leftMargin:     control._spacing
        anchors.left:           attitude.right
        size:                   control._innerRadius * 2
        vehicle:                globals.activeVehicle
        anchors.verticalCenter: parent.verticalCenter
    }
}
