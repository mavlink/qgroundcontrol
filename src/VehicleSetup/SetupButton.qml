import QtQuick 2.2
import QtQuick.Controls 1.2

Rectangle {
    property alias title: titleBar.text
    border.color: "#888"
    radius: 10
    property real titleHeight: 30

    gradient: Gradient {
        GradientStop { position: 0 ; color: "#cccccc" }
        GradientStop { position: 1 ; color: "#aaa" }
    }

    Text {
        id: titleBar
        width: parent.width
        height: parent.titleHeight
        verticalAlignment: TextEdit.AlignVCenter
        horizontalAlignment: TextEdit.AlignHCenter
        text: qsTr("TITLE")
        font.pixelSize: 12
    }

    Rectangle {
        width: parent.width
        height: parent.height - parent.titleHeight
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#ffffff"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }
        y: parent.titleHeight
    }
}
