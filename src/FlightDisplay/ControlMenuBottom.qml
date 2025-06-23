import QtQuick
import QtQuick.Controls

Rectangle {
    id: _root
    width: 512
    height: 80
    color: "transparent"

    // --- Các hàm xử lý toggle
    function toggleBtn1() {
        btn1.isActive = !btn1.isActive
        console.log("Button 1 clicked")
    }

    function toggleBtn2() {
        btn2.isActive = !btn2.isActive
        console.log("Button 2 clicked")
    }

    function toggleBtn3() {
        btn3.isActive = !btn3.isActive
        console.log("Button 3 clicked")
    }

    function toggleBtn4() {
        btn4.isActive = !btn4.isActive
        console.log("Button 4 clicked")
    }

    function toggleBtn5() {
        btn5.isActive = !btn5.isActive
        console.log("Button 5 clicked")
    }

    function toggleBtn6() {
        btn6.isActive = !btn6.isActive
        console.log("Button 6 clicked")
    }
    function toggleBtn7() { btn7.isActive = !btn7.isActive; console.log("Button 7 clicked") }
    function toggleBtn8() { btn8.isActive = !btn8.isActive; console.log("Button 8 clicked") }
    function toggleBtnStartMission() { btnStartMission.isActive = !btnStartMission.isActive; console.log("Start Mission clicked") }

    function toggleBtnCoiBao() { 
        btnCoiBao.isActive = !btnCoiBao.isActive
        QGroundControl.CustomCommandManager.triggerCoiBao()

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
                // iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtnStartMission()
            }

            ItemButton {
                id: btnCoiBao
                width: 100
                label: "còi báo"
                iconSource: "/icons/campaign_while.svg"
                onClicked: toggleBtnCoiBao()
            }

            ItemButton {
                id: btn8
                label: "8"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn8()
            }
            ItemButton {
                id: btn7
                label: "7"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn7()
            }
        }

        Row {
            spacing: 10

            ItemButton {
                id: btn1
                label: "1"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn1()
            }

            ItemButton {
                id: btn2
                label: "2"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn2()
            }

            ItemButton {
                id: btn3
                label: "3"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn3()
            }

            ItemButton {
                id: btn4
                label: "4"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn4()
            }
            ItemButton {
                id: btn5
                label: "5"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn5()
            }
            ItemButton {
                id: btn6
                label: "6"
                iconSource: "/icons/rocket_while.svg"
                onClicked: toggleBtn6()
            }
        }
    }
    
}
