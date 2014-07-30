import QtQuick 2.1

Rectangle {
    id: statusDisplay
    property alias statusText: armedText.text
    property alias statusTextColor: armedText.color
    property alias statusBackgroundColor: statusDisplay.color

    width: 100
    height: parent.height/3
    anchors.verticalCenter: parent.verticalCenter
    radius: 3
    border.color: "white"
    border.width: 1

    Text {
        id: armedText
        anchors.centerIn: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 20
    }

}
