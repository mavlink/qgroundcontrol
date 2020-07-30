/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtQuick.Controls         2.4
import QtQuick.Layouts          1.12

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

/// Multi vehicle panel for Fly View
Item {
    id:         _root
    height:     singleVehiclePanel ? selectorRow.height : availableHeight
    visible:    QGroundControl.multiVehicleManager.vehicles.count > 1 && QGroundControl.corePlugin.options.flyView.showMultiVehicleList

    property alias  singleVehiclePanel:     singleVehicleView.checked
    property real   availableHeight

    QGCMapPalette { id: mapPal; lightColors: true }

    Row {
        id:         selectorRow
        spacing: ScreenTools.defaultFontPixelWidth

        QGCRadioButton {
            id:             singleVehicleView
            text:           qsTr("Single")
            checked:        true
            textColor:      mapPal.text
        }

        QGCRadioButton {
            text:           qsTr("Multi-Vehicle")
            textColor:      mapPal.text
        }
    }

    MultiVehicleList {
        anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
        anchors.top:        selectorRow.bottom
        anchors.bottom:     parent.bottom
        width:              parent.width
        visible:            !singleVehiclePanel && !QGroundControl.videoManager.fullScreen && QGroundControl.corePlugin.options.showMultiVehicleList
    }
}
