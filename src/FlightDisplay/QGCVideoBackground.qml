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

Item {
    id: videoBackground

    property string streamObjectName: ""

    Rectangle {
        anchors.fill: parent
        color: "black"
    }

    Loader {
        id: gstViewLoader
        anchors.fill: parent
        active: QGroundControl.videoManager.hasVideo && QGroundControl.videoManager.gstreamerEnabled
        source: "qrc:/qml/QGroundControl/FlightDisplay/FlightDisplayViewGStreamer.qml"
        onLoaded: {
            if (item && streamObjectName.length) {
                item.objectName = streamObjectName
            }
        }
    }
}
