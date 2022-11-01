import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

// Qml definition of a single Frame/group view
// It will display the `Frames*` object

Rectangle {
    id: frameView
    // Frame* pointer we are referencing to
    property var frame
    // Defines whether this frame is selected by the user (if it is an End node)
    property bool selected: false

    width: _boxWidth; height: _boxHeight
    color: frame.isEndNode ? "#00000000" : "#20000000" // Transparent if End-node

    // Click border
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            // We directly call the `frameComponent` object that exists in the `Main` QML
            // This is a bad practice, but since having a MouseArea defined in the `Main` QML
            // somehow disables clicking on the product URL, this decision had to be made.
            frameComponent.selectFrame(modelData)
        }
    }

    // Main Column
    Column {
        anchors.fill: parent
        spacing: 0

        // Name
        Rectangle {
            id: frameNameBackground
            color: "#00000000" // Transparent
            width: parent.width
            height: parent.height * 0.1

            Text {
                id: frameName
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pointSize: _defaultFontPointSize * 1.5
                text: frame.name
            }
        }

        // Everything below the name
        Rectangle {
            color: selected ? "yellow" : "lightgreen"
            width: parent.width
            height: parent.height * 0.9

            Column {
                spacing: 0
                anchors.fill: parent

                // Image
                Rectangle {
                    color: "#20FFFFFF" // Tinted semi-trasnparent bright background
                    width: parent.width
                    height: parent.height * 0.7

                    Image {
                        id: frameImage
                        anchors.fill: parent
                        anchors.margins: _boxSpacing // Have some margins for image
                        source: frame.image_url
                        fillMode: Image.PreserveAspectFit
                    }
                }

                // Description
                Rectangle {
                    color: "#80FFFFFF" // Tinted semi-trasnparent bright background
                    width: parent.width
                    height: parent.height * 0.2

                    Text {
                        id: frameDescription
                        anchors.fill: parent
                        anchors.margins: _boxSpacing // Have some margins for description
                        font.pointSize: _defaultFontPointSize
                        wrapMode: Text.WordWrap
                        text: frame.description
                    }
                }

                Row {
                    id: frameBottomRow
                    spacing: 0
                    width: parent.width
                    height: parent.height * 0.1

                    // Manufacturer
                    Rectangle {
                        color: "#90FFFFFF" // Tinted semi-trasnparent bright background
//                        width: parent.width * 0.5
                        height: parent.height
//                        width: frameManufacturer.implicitWidth
                        width: parent.width * 0.5

                        Text {
                            id: frameManufacturer
                            anchors.fill: parent
                            font.pointSize: _defaultFontPointSize
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: frame.manufacturer
                        }
                    }

                    // URL
                    Rectangle {
                        color: "#90FFFFFF" // Tinted semi-trasnparent bright background
                        height: parent.height
//                        width: frameUrl.implicitWidth
                        width: parent.width * 0.5

                        Label {
                            id: frameUrl
                            anchors.fill: parent
                            font.pointSize: _defaultFontPointSize
                            text: "<a href='" + frame.product_url + "'>" + "link to product</a>"
                            visible: frame.product_url ? true : false
                            color: "blue"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            onLinkActivated: {
                                console.log("Trying to open: " + frame.product_url)
                                Qt.openUrlExternally(frame.product_url)
                            }
                        }
                    }
                }
            }
        }
    }
}
