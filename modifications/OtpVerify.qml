import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.0
import QtQuick.Dialogs 1.1

import QGroundControl 1.0
Rectangle{
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property bool vis: false
    signal backButtonClicked()
    property string otpVal
    signal getOtpVal()
    signal setOtp(string a)
    signal verifyButton()
    id:myrect
    anchors.fill: parent
    color: "grey"
    z:1
    visible: myrect.vis && _activeVehicle&&!_communicationLost


Rectangle{
    id: rect3
    width:parent.width/3
    height:parent.height/2
    anchors.centerIn: parent
    radius: 80
    color:"lightgrey"
    border.color:"black"
    border.width: 3


    Text {
        id: otptxt
        text: qsTr("<h2>Enter the OTP sent to your Registered Email<\h2>")
        anchors.centerIn: rect3
        anchors.verticalCenterOffset: -150
    }

    TextField{
        id:otpedit
        anchors.verticalCenterOffset: -34
        anchors.horizontalCenterOffset: 0
        placeholderText: qsTr("Enter OTP")
        text : otpVal
        onTextChanged: {
            otpVal = text;
            setOtp(text);
            getOtpVal();
        }

        anchors.centerIn: rect3
    }

    Button{
        id: verifyOTP
        text: "<b>Verify<\b>"
        anchors.centerIn: rect3
        anchors.verticalCenterOffset: 112
        anchors.horizontalCenterOffset: 80
        onClicked: verifyButton()
}


    Button{
        id: backbtn
        text: "<b>BACK<\b>"
        anchors.centerIn: rect3
        anchors.verticalCenterOffset: 112
        anchors.horizontalCenterOffset: -80
        onClicked: backButtonClicked()
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
