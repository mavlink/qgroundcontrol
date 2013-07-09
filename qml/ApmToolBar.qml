import QtQuick 1.1
import "./components"


Rectangle {
    id: toolbar
    width: parent.width
    height: 72
    color: "black"
    border.color: "black"

    Row {
        anchors.left: parent.left
        spacing: 2

        Button {
            id: flightDataView
            label: "FLIGHT DATA"
            image: "./resources/apmplanner/toolbar/flightdata.png"
            onClicked: {
                globalObj.selectFlightView()
            }
        }

        Button {
            id: flightPlanView
            label: "FLIGHT PLAN"
            image: "./resources/apmplanner/toolbar/flightplanner.png"
            onClicked: globalObj.selectFlightPlanView()
        }

        Button {
            id: hardwareConfigView
            label: "HARDWARE"
            image: "./resources/apmplanner/toolbar/hardwareconfig.png"
            margins: 8
            onClicked: globalObj.selectHardwareView()
        }

        Button {
            id: softwareConfigView
            label: "SOFTWARE"
            image: "./resources/apmplanner/toolbar/softwareconfig.png"
            margins: 8
            onClicked: globalObj.selectSoftwareView()
        }

        Button {
            id: simualtionView
            label: "SIMULATION"
            image: "./resources/apmplanner/toolbar/simulation.png"
            onClicked: globalObj.selectSimulationView()
        }

        Button {
            id: terminalView
            label: "TERMINAL"
            image: "./resources/apmplanner/toolbar/terminal.png"
            onClicked: globalObj.selectTerminalView()
        }
    }

    Row {
        anchors.left: parent.right
        spacing: 2

        Button {
            id: connectButton
            label: "CONNECT"
            image: "./resources/apmplanner/toolbar/connect.png"
            onClicked: globalObj.connect()
        }
    }
}

