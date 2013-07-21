import QtQuick 1.1

Rectangle {

    property alias title: displayTitle.text
    property string textValue: "none"

    width: 100
    height: parent.height/3
    anchors.verticalCenter: parent.verticalCenter
    border.color: "white"

    Text {
        id:displayTitle
        anchors.left: parent.left
        anchors.leftMargin: 3
        anchors.verticalCenter: parent.verticalCenter
        text: "blank"
        color: "white"
    }

    Text {
        id:displayValue
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter
        text: textValue
        color: "white"
    }
}
