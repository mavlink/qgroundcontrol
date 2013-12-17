import QtQuick 1.1

Rectangle {
    signal clicked

    property string label: "Text Button label"
    property int minWidth: 75
    property int minHeight: 0
    property int margin: 5

    width: textBox.width
    height: 72
    anchors.verticalCenter: parent.verticalCenter
    color: "black"

    signal buttonClick()

    onButtonClick: {
        console.log(label + " clicked calling signal")
        clicked()
    }

    // Highlighting and ativation section
    property color buttonColor: "black"
    property color onHoverbuttonColor: "lightblue"
    property color onHoverColor: "darkblue"
    property color borderColor: "white"

    Rectangle {
        width: textButtonLabel.paintedwidth
        anchors.centerIn: parent

        Rectangle{
            id: textBox
            anchors.centerIn: parent
            width: minWidth > textButtonLabel.paintedWidth + margin ? minWidth : textButtonLabel.paintedWidth + margin
            height: minHeight > textButtonLabel.paintedHeight + margin ? minHeight : textButtonLabel.paintedHeight + margin

            Text {
                id: textButtonLabel
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.margins: 2
                text: label
                color: "white"
                font.pointSize: 11
            }

            MouseArea {
                id: textButtonMouseArea
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
            border.width: 1
        }
   }
}
