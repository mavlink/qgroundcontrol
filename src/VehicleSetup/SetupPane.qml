import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
//import QGroundControl.FactControls 1.0

Item {
    id: item1
    width: 500
    height: 500

    Rectangle {
        id: innerRect
        color: "#d298d2"
        z: 1
        anchors.rightMargin: 15
        anchors.leftMargin: 15
        anchors.bottomMargin: 15
        anchors.topMargin: 40
        anchors.fill: parent

        Rectangle {
            id: close
            x: parent.width - (width / 2)
            y: 0 - (height / 2)
            width: 30
            height: 30
            color: "#ffffff"
            radius: 15
            z: 2
            border.color: "#000000"
        }

    }

    Rectangle {
        id: outerRect
        color: "#ffffff"
        opacity: 0.8
        anchors.fill: parent

        Text {
            id: title
            x: 237
            y: 8
            text: qsTr("Setup Pane")
            anchors.horizontalCenter: parent.horizontalCenter
            font.pointSize: 20
        }
    }

}
