import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0

Rectangle {
    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    id: topLevel
    objectName: "topLevel"

    anchors.fill: parent
    color: palette.window

    signal buttonClicked(variant component);

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: autopilot.setupBackgroundImage;
    }

    Column {
        anchors.margins: 20
        anchors.fill: parent
        spacing: 5

        Rectangle { id: header; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.8;
            Text { id: titleText; anchors.centerIn: parent; font.pointSize: 24; text: "Vehicle Summary" }
        }

        Flow {
            width: parent.width;
            height: parent.height - header.height - footer.height
            spacing: 5

            Repeater {
                model: autopilot.components

                Button {
                    width: 250
                    height: 200

                    property var summaryModel: modelData.summaryItems
                    text: modelData.name
                    property bool setupComplete: modelData.setupComplete

                    style: ButtonStyle {
                        id: buttonStyle
                        background: Rectangle {
                            id: innerRect
                            readonly property real titleHeight: 30

                            border.color: "#888"
                            radius: 10

                            color: control.activeFocus ? "#47b" : "white"
                            opacity: control.hovered || control.activeFocus ? 1 : 0.8
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
                                    anchors.margins: 4
                                    model: control.summaryModel
                                        delegate: Row { width: parent.width
                                            Text { id: firstCol; text: modelData.name }
                                            Text { horizontalAlignment: Text.AlignRight; width: parent.width - firstCol.contentWidth; text: modelData.state }
                                    }
                                }
                            }
                        }

                    label: Item {}
                    }

                onClicked: topLevel.buttonClicked(modelData)
                }
            }
        }

        Rectangle { id: footer; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.8;

            property real spacing: (width - firmwareButton.width - parametersButton.width) / 3

            Button { id: firmwareButton; objectName: "firmwareButton";
                x: parent.spacing; anchors.verticalCenter: parent.verticalCenter;
                text: "Firmware Upgrade" }
            Button { id: parametersButton; objectName: "parametersButton"
                x: firmwareButton.width + (parent.spacing*2); anchors.verticalCenter: parent.verticalCenter;
                text: "Parameters" }
        }

    }
}
