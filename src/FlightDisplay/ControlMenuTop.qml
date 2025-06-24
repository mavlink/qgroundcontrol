import QtQuick
import QtQuick.Controls
import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: _root
    width: 512
    height: 80
    color: "transparent"

    // Tab hiện tại đang chọn
    property string currentTab: "Home"

    function switchTab(tabId) {
        currentTab = tabId
        console.log("🔁 Switched to tab:", tabId)
    }

    Column {
        spacing: 10
        anchors.centerIn: parent

        Row {
            spacing: 10

            ItemButton {
                id: btnHome
                width: 100
                label: "Home"
                isActive: currentTab === "Home"
                onClicked: switchTab("Home")
            }

            ItemButton {
                id: btnTelemetry
                width: 100
                label: "Telemetry"
                isActive: currentTab === "Telemetry"
                onClicked: switchTab("Telemetry")
            }

            ItemButton {
                id: btnMission
                width: 100
                label: "Mission"
                isActive: currentTab === "Mission"
                onClicked: switchTab("Mission")
            }

            ItemButton {
                id: btnCamera
                width: 100
                label: "Camera"
                isActive: currentTab === "Camera"
                onClicked: switchTab("Camera")
            }
        }

        // Hiển thị trạng thái tab hiện tại (để bạn debug)
        // Text {
        //     text: "Current Tab: " + currentTab
        //     color: "white"
        //     font.pixelSize: 14
        //     anchors.horizontalCenter: parent.horizontalCenter
        // }
    }
}
