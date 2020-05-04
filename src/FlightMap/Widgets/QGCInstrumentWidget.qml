/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.Palette       1.0

ColumnLayout {
    id:         root
    width:      getPreferredInstrumentWidth()
    spacing:    ScreenTools.defaultFontPixelHeight / 4

    property real   _innerRadius:           (width - (_topBottomMargin * 3)) / 4
    property real   _outerRadius:           _innerRadius + _topBottomMargin
    property real   _defaultSize:           ScreenTools.defaultFontPixelHeight * (9)
    property real   _sizeRatio:             ScreenTools.isTinyScreen ? (width / _defaultSize) * 0.5 : width / _defaultSize
    property real   _bigFontSize:           ScreenTools.defaultFontPointSize * 2.5  * _sizeRatio
    property real   _normalFontSize:        ScreenTools.defaultFontPointSize * 1.5  * _sizeRatio
    property real   _labelFontSize:         ScreenTools.defaultFontPointSize * 0.75 * _sizeRatio
    property real   _spacing:               ScreenTools.defaultFontPixelHeight * 0.33
    property real   _topBottomMargin:       (width * 0.05) / 2
    property real   _availableValueHeight:  maxHeight - _valuesItem.y

    QGCPalette { id: qgcPal }

    Rectangle {
        id:                 visualInstrument
        height:             _outerRadius * 2
        Layout.fillWidth:   true
        radius:             _outerRadius
        color:              qgcPal.window
        border.width:       1
        border.color:       qgcPal.mapWidgetBorderLight

        DeadMouseArea { anchors.fill: parent }

        QGCAttitudeWidget {
            id:                     attitude
            anchors.leftMargin:     _topBottomMargin
            anchors.left:           parent.left
            size:                   _innerRadius * 2
            vehicle:                activeVehicle
            anchors.verticalCenter: parent.verticalCenter
        }

        QGCCompassWidget {
            id:                     compass
            anchors.leftMargin:     _spacing
            anchors.left:           attitude.right
            size:                   _innerRadius * 2
            vehicle:                activeVehicle
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    TerrainProgress {
        Layout.fillWidth: true
    }

    Item {
        id:                 _valuesItem
        Layout.fillWidth:   true
        height:             _valuesWidget.height
        visible:            widgetRoot.showValues

        DeadMouseArea { anchors.fill: parent }

        Rectangle {
            anchors.fill:   _valuesWidget
            color:          qgcPal.window
        }

        PageView {
            id:                 _valuesWidget
            anchors.margins:    1
            anchors.left:       parent.left
            anchors.right:      parent.right
            maxHeight:          _availableValueHeight
        }
    }
}
