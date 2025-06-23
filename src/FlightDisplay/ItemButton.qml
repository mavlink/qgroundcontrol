import QtQuick
import QtQuick.Controls

Rectangle {
    id: button
    width: 50
    height: 50
    radius: 10

    // Các thuộc tính bên ngoài
    property alias label: labelText.text
    property string iconSource: ""
    property color activeColor: "#c6004f98"       // Màu khi active
    property color defaultColor: "#bf222222"      // Màu mặc định
    signal clicked()

    // Trạng thái nội bộ
    property bool isActive: false

    color: isActive ? activeColor : defaultColor

    // Khi có icon, hiển thị icon + label trong Column
    // Khi không có icon, chỉ hiển thị label và căn giữa
    Item {
        anchors.fill: parent

        // Nếu có icon
        Column {
            id: contentWithIcon
            anchors.centerIn: parent
            spacing: 4
            visible: iconSource !== ""

            Image {
                source: iconSource
                width: 24
                height: 24
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                id: labelText
                text: "1"
                font.pixelSize: 12
                color: "white"
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Nếu không có icon, chỉ hiển thị chữ ở giữa
        Text {
            id: labelOnlyText
            visible: iconSource === ""
            anchors.centerIn: parent
            text: labelText.text
            font.pixelSize: 14
            color: "white"
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: button.clicked()
    }

    Behavior on color {
        ColorAnimation { duration: 150 }
    }
}
