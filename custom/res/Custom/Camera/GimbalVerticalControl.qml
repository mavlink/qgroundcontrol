import QtQuick 2.0

Item {
    id: _root

    property alias radius: backgroundRectangle.radius

    property color mainColor: "#000000"

    Rectangle {
        id: backgroundRectangle

        anchors.fill: parent
        color: _root.mainColor
    }
}
