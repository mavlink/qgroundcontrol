import QtQuick 2.4
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.3
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.1

import org.freedesktop.gstreamer.GLVideoItem 1.0

ApplicationWindow {
    id:             multiVideoWindow
    title:          "Multi-Video Context"
    minimumWidth:   300
    minimumHeight:  400
    width:          1280
    height:         720
    visible:        true

    Item {
        anchors.fill: parent
        GstGLVideoItem {
            id: videoBackground
            objectName: "videoContent"
            anchors.centerIn: parent
            width: parent.width
            height: parent.height
            property var receiver
        }
    }
}
