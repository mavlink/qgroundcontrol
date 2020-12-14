import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.1

import QGroundControl 1.0
Rectangle{
    id:myrect
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property bool check1:false
    property bool check2:false
    property bool check3:false
    property bool check4:false
    property bool vis: false
    anchors.fill: parent
    color: "grey"
    visible: myrect.vis &&_activeVehicle&&!_communicationLost
    z:1

Rectangle{
    id: rect2
    width:parent.width/3
    height:parent.height/2
    anchors.centerIn: parent
    radius: 80
    color:"lightgrey"
    border.color:"black"
    border.width: 3
    MouseArea{
        anchors.fill:parent
        z:2
    }

    Column{

        spacing: 20
        anchors.centerIn: rect2

       //START NPNT CHECKLIST
        Text {
            id: init
            text: qsTr("<h1>STARTING INIT PROCESS..<\h1>")
        }

        Column{
                CheckBox {
                    id: c1
                    checked:myrect.check1
                    text: qsTr("Hardware Connected")
                    checkable:false

                }
                CheckBox {
                    id:c2
                    checked: myrect.check2
                    text: qsTr("Check if Drone is Registered")
                    checkable:false
                }
                CheckBox {
                    id:c3
                    checked: myrect.check3
                    text: qsTr("Check for Firmware Upgrades")
                    checkable:false



                }
                CheckBox {
                    id:c4
                    checked: myrect.check4
                    text: qsTr("Starting Key Rotation")
                    checkable:false

                }

        }

    }

}
Image {
    id: logo
    source: "/qmlimages/cropped-SpaceJamUAV-Logo-2048x663.png"
    fillMode: Image.PreserveAspectFit
    width: parent.width/3
    height: parent.height/6
}
}
