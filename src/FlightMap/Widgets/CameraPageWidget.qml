/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

/// Camera page for Instrument Panel PageView
Column {
    width:      pageWidth
    spacing:    ScreenTools.defaultFontPixelHeight

    property bool showSettingsIcon: false

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    QGCButton {
        anchors.horizontalCenter:   parent.horizontalCenter
        text:                       qsTr("Trigger Camera")
        onClicked:                  _activeVehicle.triggerCamera()
        enabled:                    _activeVehicle
    }
}
