import QtQuick 1.1

Rectangle {
    signal clicked

    property string label: "button label"
    property alias image: buttonImage.source
    property int margins: 2

    id: button
    width: 72
    height: 72
    radius: 3
    smooth: true
    border.width: 2

    Text {
        id: buttonLabel
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 5
        text: label
        color: "white"
        font.pointSize: 10
    }

    Image {
        id: buttonImage
        anchors.horizontalCenter: button.horizontalCenter
        anchors.top: buttonLabel.bottom
        anchors.margins: margins
        source: image
        fillMode: Image.PreserveAspectFit
        width: image.width
        height: image.height
    }

    signal buttonClick()

    onButtonClick: {
        console.log(buttonLabel.text + " clicked calling signal")
        clicked()
    }

    // Highlighting and ativation section
    property color buttonColor: "black"
    property color onHoverbuttonColor: "lightblue"
    property color onHoverColor: "darkblue"
    property color borderColor: "black"

    MouseArea {
        id: buttonMouseArea
        anchors.fill: parent
        onClicked: buttonClick()
        hoverEnabled: true
        onEntered: {
            parent.border.color = onHoverColor
            parent.color = onHoverbuttonColor
        }
        onExited: {
            parent.border.color = borderColor
            parent.color = buttonColor
        }
        onPressed: parent.color = Qt.darker(onHoverbuttonColor, 1.5)
        onReleased: parent.color = buttonColor
    }
    color: buttonColor
    border.color: borderColor
}

