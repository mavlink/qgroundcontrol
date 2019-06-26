import QtQuick 2.0

Item {
    property real scale: 1.0
    property color mainColor: "darkblue"
    property color secondaryColor: "darkblue"

    Rectangle {
        id: joystickBk
        width: scale * 100
        height: width
        radius: 0.5

        color: mainColor

        Rectangle {
            width: parent.width/4
            height: width
            radius: 0.5
            anchors.centerIn: parent

            color: secondaryColor
        }
    }
}
