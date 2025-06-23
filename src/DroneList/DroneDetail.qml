import QtQuick 
import QtQuick.Controls 
import QtQuick.Layouts 
import QtQuick3D
import Qt5Compat.GraphicalEffects

Rectangle {
    id: detailPage
    // anchors.fill: parent
    // z: 0

    property var uavData
    property var onBack: function() {}
    property real dragX: 0
    property real dragY: 0

    // Nút quay lại
    Button {
        text: "← Back"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        onClicked: onBack()
    }

    // Tên & mã code
    Column {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 4
        Text {
            text: uavData.name
            font.bold: true
            font.pointSize: 20
        }
        Text {
            text: uavData.code
            font.pointSize: 14
        }
    }

    // Ảnh UAV
    Image {
        source: uavData.image
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 24
        width: 200
        height: 160
        fillMode: Image.PreserveAspectFit
    }

    // Mô tả
    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 180
        anchors.rightMargin: 24
        width: 200
        height: 150
        radius: 16
        color: "#eeeeee"
        Text {
            anchors.centerIn: parent
            width: parent.width * 0.9
            wrapMode: Text.WordWrap
            text: uavData.description ?? "No description"
        }
    }

    // Thông số kỹ thuật
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 24
        anchors.topMargin: 80
        width: 220
        height: 300
        radius: 16
        color: "#f5f5f5"
        ScrollView {
            anchors.fill: parent
            Column {
                spacing: 6
                Repeater {
                    model: Object.keys(uavData).filter(k => !["name", "code", "image", "description", "link"].includes(k))
                    delegate: Text {
                        text: `${modelData}: ${uavData[modelData]}`
                        font.pointSize: 12
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    // Mô hình 3D

    View3D {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        camera: camera

        Node {
            id: sceneRoot

            PerspectiveCamera {
                id: camera
                position: Qt.vector3d(0, 0, 600)
            }

            DirectionalLight {
                eulerRotation: Qt.vector3d(45, 45, 0)
            }

            Model {
                id: model3d
                source: "#Sphere"
                scale: Qt.vector3d(100, 100, 100)
                eulerRotation: Qt.vector3d(detailPage.dragY, detailPage.dragX, 0)
                materials: DefaultMaterial {
                    diffuseColor: "orange"
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            property real lastX
            property real lastY

            onPressed: {
                lastX = mouse.x
                lastY = mouse.y
            }

            onPositionChanged: {
                detailPage.dragX += (mouse.x - lastX)
                detailPage.dragY += (mouse.y - lastY)
                lastX = mouse.x
                lastY = mouse.y
            }
        }
    }

    // Button liên hệ & xác nhận
    Row {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        spacing: 12
        anchors.bottomMargin: 16
        anchors.rightMargin: 16

        Button {
            text: "Thông tin liên hệ"
            onClicked: Qt.openUrlExternally(uavData.link)
        }
        Button {
            text: "Xác nhận"
            onClicked: console.log("Xác nhận UAV:", uavData.name)
        }
    }
}
