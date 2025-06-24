import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: _root
    width: 512
    height: 80
    color: "transparent"

    visible: QGroundControl.multiVehicleManager.activeVehicle !== null


    function sendCustomMavCommand(btn_id, mavCmdId, param1 = 1) {
        let vehicle = QGroundControl.multiVehicleManager.activeVehicle
        if (vehicle) {
            btn_id.isActive = !btn_id.isActive
            vehicle.sendCommand(
                vehicle.id,
                mavCmdId,
                true,
                param1, 0, 0, 0, 0, 0, 0
            )
            console.log("📡 Send MAV_CMD " + mavCmdId)
        } else {
            console.warn("🚫 error")
        }
    }

    Column {
        spacing: 10
        anchors.centerIn: parent

        Row {
            spacing: 10

            ItemButton {
                id: btnStartMission
                width: 170
                label: "Start Mission"
                onClicked: sendCustomMavCommand(btnStartMission, 30000) 
            }

            ItemButton {
                id: btnCoiBao
                width: 110
                label: "Còi báo"
                iconSource: "/icons/campaign_while.svg"
                onClicked: btnCoiBao.isActive ? sendCustomMavCommand(btnCoiBao, 30009) : sendCustomMavCommand(btnCoiBao, 30001)
            }

            ItemButton {
                id: btn7
                label: "7"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn7, 30007)
            }
        }

        Row {
            spacing: 10

            ItemButton {
                id: btn1
                label: "1"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn1, 30001)
            }

            ItemButton {
                id: btn2
                label: "2"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn2, 30002)
            }

            ItemButton {
                id: btn3
                label: "3"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn3, 30003)
            }

            ItemButton {
                id: btn4
                label: "4"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn4, 30004)
            }

            ItemButton {
                id: btn5
                label: "5"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn5, 30005)
            }

            ItemButton {
                id: btn6
                label: "6"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn6, 30006)
            }
        }
    }
}
