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
    property string currentTab: "POWERLINE"

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
                id: btnPOWERLINE
                defaultColor: "transparent"
                activeColor: "#d1222222"
                width: 100
                height: 30
                radius: 6
                bold: true
                fontSize: 14
                label: "POWERLINE"
                isActive: currentTab === "POWERLINE"
                onClicked: switchTab("POWERLINE")
            }

            ItemButton {
                id: btnCONSTRUCTION
                defaultColor: "transparent"
                activeColor: "#d1222222"
                width: 140
                height: 30
                radius: 6
                bold: true
                fontSize: 14
                label: "CONSTRUCTION"
                isActive: currentTab === "CONSTRUCTION"
                onClicked: switchTab("CONSTRUCTION")
            }

            ItemButton {
                id: btnTOURISM
                defaultColor: "transparent"
                activeColor: "#d1222222"
                width: 100
                height: 30
                radius: 6
                bold: true
                fontSize: 14
                label: "TOURISM"
                isActive: currentTab === "TOURISM"
                onClicked: switchTab("TOURISM")
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
