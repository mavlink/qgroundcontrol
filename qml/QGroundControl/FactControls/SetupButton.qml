import QtQuick 2.2
import QtQuick.Controls 1.2

Rectangle {
    readonly property real titleHeight: 30

    property alias title: titleBar.text
    property alias setupComplete: setupIndicator.setupComplete
    property alias summaryModel: summaryList.model

    border.color: "#888"
    radius: 10

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

        Rectangle {
            id: setupIndicator

            property bool setupComplete: true
            readonly property real indicatorRadius: 6

            x: parent.width - (indicatorRadius * 2) - 5
            y: (parent.height - (indicatorRadius * 2)) / 2
            width: indicatorRadius * 2
            height: indicatorRadius * 2

            radius: indicatorRadius
            color: setupComplete ? "green" : "red"
        }
    }

    Rectangle {
        width: parent.width
        height: parent.height - parent.titleHeight

        y: parent.titleHeight

        border.color: "#888"

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

        ListView {
            id: summaryList
            anchors.fill: parent
            model: ListModel {
                        ListElement { name: "Row 1"; state: "State 1" }
                        ListElement { name: "Row 2"; state: "State 2" }
                        ListElement { name: "Row 3"; state: "State 3" }
            }
            delegate: Row { Text { text: modelData.name } Text { text: modelData.state } }
        }
    }
}
