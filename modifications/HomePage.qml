import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.1
import QGroundControl 1.0
Rectangle{
        id:myRect
        color:"grey"
        anchors.fill: parent
        z:1
        property bool vis:true
        visible: myRect.vis
        property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
        property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false


Image {
    id: logo
    source: "/qmlimages/cropped-SpaceJamUAV-Logo-2048x663.png"
    fillMode: Image.PreserveAspectFit
    width: parent.width/3
    height: parent.height/6

}

Rectangle {
    id: popup
    anchors.centerIn: parent
    width: parent.width/4
    height: parent.height/4
    focus: true
    radius:30
    visible: !_activeVehicle || _communicationLost
    color:"lightgrey"
    border.color:"black"
    border.width: 3

    Text {
        id: text1
        text: qsTr("<h2><b>Drone is not connected to QGC,<br> Please Turn on your Drone First !!!<\b><\h2>")
        anchors.centerIn: popup

    }

}

}


