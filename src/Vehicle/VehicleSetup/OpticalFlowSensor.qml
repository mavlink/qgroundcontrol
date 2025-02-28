/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
    QGCLabel {
        text: qsTr("Optical Flow Camera")
        font.bold: true
    }

    Image {
        source: globals.activeVehicle ? "image://QGCImages/" + globals.activeVehicle.id + "/" + globals.activeVehicle.flowImageIndex : ""
        width: parent.width * 0.5
        height: width * 0.75
        cache: false
        fillMode: Image.PreserveAspectFit
        anchors.centerIn: parent
    }
}
