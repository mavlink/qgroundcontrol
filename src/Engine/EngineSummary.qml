import QtQuick
import QtQuick.Shapes
import QtQuick.Layouts

Item {
    id: root
    anchors.fill: parent

    component ToggleSwitchBlue: Rectangle {
        id: toggleBlue
        width: 180
        height: 60
        radius: 6
        color: checked ? "#42A5F5" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleBlue.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleBlue.checked = !toggleBlue.checked
        }
    }

    component ToggleSwitchGreen: Rectangle {
        id: toggleGreen
        width: 180
        height: 60
        radius: 6
        color: checked ? "#4CAF50" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleGreen.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleGreen.checked = !toggleGreen.checked
        }
    }

    component ToggleSwitchRed: Rectangle {
        id: toggleRed
        width: 180
        height: 60
        radius: 6
        color: checked ? "#424242" : "#af504c"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleRed.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleRed.checked = !toggleRed.checked
        }
    }
    component ToggleSwitchAmber: Rectangle {
        id: toggleAmber
        width: 180
        height: 60
        radius: 6
        color: checked ? "#ffbf00" : "#424242"
        border.color: "#666"
        border.width: 1

        property string text: ""
        property bool checked: false

        Text {
            anchors.centerIn: parent
            text: toggleAmber.text
            color: "white"
            font.bold: true
            font.pixelSize: 15
        }

        MouseArea {
            anchors.fill: parent
            onClicked: toggleAmber.checked = !toggleAmber.checked
        }
    }


    component BarGauge: Rectangle {
        id: gauge
        width: 70
        height: 240
        color: "#2a2a2a"
        border.color: "#555"
        border.width: 1
        radius: 4

        property string label: ""
        property real value: 0
        property real maxValue: 100
        property string unit: ""
        property color barColor: "#2196F3"
        property real percentage: Math.min(Math.max(value / maxValue, 0), 1)

        Column {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 6

            Text {
                width: parent.width
                text: gauge.value.toFixed(0)
                color: "white"
                font.bold: true
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                height: 16
            }

            Text {
                width: parent.width
                text: gauge.unit
                color: "#aaa"
                font.pixelSize: 10
                horizontalAlignment: Text.AlignHCenter
                height: 12
            }

            Item {
                width: parent.width
                height: parent.height - 76
                anchors.horizontalCenter: parent.horizontalCenter

                Rectangle {
                    anchors.fill: parent
                    color: "#1a1a1a"
                    radius: 2
                }

                Rectangle {
                    width: parent.width - 10
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: parent.height * gauge.percentage
                    radius: 2
                    color: gauge.percentage > 0.8 ? "#f44336" : gauge.barColor
                }

                Repeater {
                    model: 5
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#444"
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: parent.height / 4 * index
                        opacity: 0.5
                    }
                }
            }

            Text {
                width: parent.width
                height: 40
                text: gauge.label
                color: "white"
                font.bold: true
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                lineHeight: 1.2
            }
        }
    }

    // 左上角开关布局
    Column {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 20
        spacing: 20

        ToggleSwitchAmber { text: "Fuel Pump 1" }
        ToggleSwitchAmber { text: "Fuel Pump 2"}
        ToggleSwitchBlue { text: "Water Cooler Fan 1"}
        ToggleSwitchBlue { text: "Water Cooler Fan 2" }
        ToggleSwitchBlue { text: "Inter Cooler Fan" }
        ToggleSwitchGreen { text: "Engine Start" }
        ToggleSwitchRed   { text: "Emergency STOP"}
    }

    // 右侧横向仪表布局
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 20
        spacing: 20

        BarGauge {
            id: rpmGauge
            label: "Engine\nRPM"
            value: 2500
            maxValue: 6000
            unit: "rpm"
            barColor: "#42A5F5"
        }

        BarGauge {
            id: intakeGauge
            label: "Intake\nTemp"
            value: 45
            maxValue: 100
            unit: "°C"
            barColor: "#FFA726"
        }

        BarGauge {
            id: exhaustGauge
            label: "Exhaust\nTemp"
            value: 720
            maxValue: 900
            unit: "°C"
            barColor: "#EF5350"
        }

        BarGauge {
            id: oilGauge
            label: "Oil\nTemp"
            value: 85
            maxValue: 120
            unit: "°C"
            barColor: "#66BB6A"
        }
    }

    // 数据更新定时器
    Timer {
        interval: 800  // 每800毫秒更新一次
        running: true
        repeat: true

        onTriggered: {
            // Engine RPM: 1000-2800 之间波动（模拟引擎转速）
            rpmGauge.value = 4000 + Math.random() * 1800

            // Intake Temp: 30-80 度之间缓慢变化
            intakeGauge.value = 30 + Math.random() * 50

            // Exhaust Temp: 600-850 度之间变化（较高温度）
            exhaustGauge.value = 600 + Math.random() * 250

            // Oil Temp: 70-110 度之间变化
            oilGauge.value = 70 + Math.random() * 40
        }
    }
}
