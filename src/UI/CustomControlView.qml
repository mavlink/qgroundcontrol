import QtQuick 2.15
import QtQuick.Controls 2.15
import QGroundControl 1.0

Item {
    id: root
    width: parent.width
    height: parent.height
    property var vehicle

    Rectangle {
        color: "#282c34"
        width: 800
        height: 600

        Text {
            anchors.centerIn: parent
            text: qsTr("欢迎来到庆军科技")
            font.pointSize: 24
            color: "white"
        }
    }
    Column {
        spacing: 20

        Text {
            text: "舵机控制（通道1）"
            font.pixelSize: 20
        }

        Slider {

            id: pwmSlider
            from: 1000
            to: 2000
            stepSize: 10
            value: 1500
            width: 200
            onValueChanged: {
                // 具体实现信号槽
                vehicle = QGroundControl.multiVehicleManager.activeVehicle
                if (vehicle) {
                    // 继承车辆对象发送指令控制pwm
                    vehicle.sendCommand(183, true, 1, value) // 通道1，PWM值
                }
            }
        }

        Text {
            text: "PWM值: " + Math.round(pwmSlider.value)
        }
    }
}

