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
                id: btn1
                width: 64
                label: "a"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn1, 30008) 
            }

            ItemButton {
                id: btn2
                width: 64
                label: "b"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn2, 30010) 
            }

            ItemButton {
                id: btn3
                width: 64
                label: "c"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn3, 30011)
            }

            ItemButton {
                id: btn4
                width: 64
                label: "d"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn4, 30012) 
            }

            ItemButton {
                id: btn5
                width: 64
                label: "e"
                iconSource: "/icons/rocket_while.svg"
                onClicked: sendCustomMavCommand(btn3, 30013)
            }
        }
    }
}
