import QtQuick
import QtQuick.Shapes 2.15
import QGroundControl

Item {
    id: root

    property int borderWidth

    QGCPalette { id: qgcPalette }

    Rectangle {
        id: border
        anchors.fill: parent
        radius: width / 2
        //color: qgcPalette.window
        color: "red"
    }

    SVZoomArea {
        id: zoomArea
        anchors.fill: parent
        anchors.margins: borderWidth
    }

    
}