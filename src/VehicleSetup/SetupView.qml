import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactControls 1.0

Rectangle {
    anchors.fill: parent
    color: "#222"

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: "octo_x.png"
    }

    Column {
        anchors.margins: 20

        anchors.fill: parent

        Rectangle { id: header; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.75
            Text { id: titleText; anchors.centerIn: parent; font.pointSize: 24; text: "Vehicle Summary" }
        }

        Flow {
        //Rectangle {
            anchors.topMargin: 10
            anchors.bottomMargin: 10

            width: parent.width;
            height: parent.height - header.height - footer.height - 200
            spacing: 5
            //color: "#028000"

            Repeater {
                model: vehicleComponents
                SetupButton {
                    width: 200
                    height: 200
                    text: modelData.name
                    setupComplete: modelData.setupComplete
                    summaryModel: modelData.summaryItems
                }
            }
        }

        Button {
            width: 200
            height: 200

            text: "Button"

            property bool setupComplete: false
            property summaryModel: ListModel {
                ListElement { name: "Row 1"; state: "State 1" }
                ListElement { name: "Row 2"; state: "State 2" }
                ListElement { name: "Row 3"; state: "State 3" }
            }

            style: ButtonStyle {
                id: buttonStyle
                background: Rectangle {
                    id: innerRect
                    readonly property real titleHeight: 30

                    property alias summaryModel: summaryList.model

                    border.color: "#888"
                    radius: 10

                    color: control.activeFocus ? "#47b" : "white"
                    opacity: control.hovered || control.activeFocus ? 1 : 0.75
                    Behavior on opacity {NumberAnimation{ duration: 100 }}

                    Text {
                        id: titleBar

                        width: parent.width
                        height: parent.titleHeight

                        verticalAlignment: TextEdit.AlignVCenter
                        horizontalAlignment: TextEdit.AlignHCenter

                        text: control.text
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
                            color: control.setupComplete ? "green" : "red"
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: parent.height - parent.titleHeight

                        y: parent.titleHeight

                        border.color: "#888"

                        gradient: Gradient {
                            GradientStop { position: 0; color: "#ffffff" }
                            GradientStop { position: 1; color: "#000000" }
                        }

                        ListView {
                            id: summaryList
                            anchors.fill: parent
                            model: control.summaryModel
                            delegate: Row { Text { text: name } Text { text: state } }
                        }
                    }
                }

            label: Item {}
            }
        }

        Row { id: footer; anchors.horizontalCenter: parent.horizontalCenter; width: parent.width; height: 50; opacity: 0.75
            Text { font.pointSize: 24; text: "Footer" }
        }
    }
}
