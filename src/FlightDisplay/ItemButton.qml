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
    property color activeColor: "#c614599b"       // Màu khi active
    property color defaultColor: "#d1222222"      // Màu mặc định
    
    property bool bold: false
    property int fontSize: 12
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
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                id: labelText
                text: "1"
                font.pixelSize: button.fontSize     // ← phải có
                font.bold: button.bold              // ← phải có
                font.family: aldrichFont.name
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                anchors.horizontalCenter: parent.horizontalCenter
            }

        }

        // Nếu không có icon, chỉ hiển thị chữ ở giữa
        Text {
            id: labelOnlyText
            visible: iconSource === ""
            anchors.centerIn: parent
            text: labelText.text
            font.pixelSize: button.fontSize     // ← phải có
            font.bold: button.bold              // ← phải có
            font.family: aldrichFont.name
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
