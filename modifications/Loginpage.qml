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
        property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
        property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
        property string usrname
        property string passwrd
        signal getUsername()
        signal getPassword()
        signal changeUsername(string a)
        signal changePassword(string b)
        signal loginButton()
        property bool vis:true
        visible: myRect.vis && _activeVehicle && !_communicationLost

Rectangle{
           id: rect1
           visible: true
           width:parent.width/3
           height:parent.height/2
           anchors.centerIn: parent
           radius: 80
           color:"lightgrey"
           border.color:"black"
           border.width: 3



           Column{
               spacing: 20
               anchors.centerIn: rect1

               Column{
               Text {
                   id: username
                   text: qsTr("Customer Email")

               }

               TextField{
                   id:customerusername
                   text: usrname
                   placeholderText: qsTr("Enter Email")
                   onTextChanged:{

                       usrname = text
                       changeUsername(text);
                       getUsername()

               }
               }

               }
               Column{

               Text {
                   id: password
                   text: qsTr("Password")

               }

               TextField{
                   id:customerpassword
                   placeholderText: qsTr("Enter Password")
                   echoMode:"Password"
                   text: passwrd
                   onTextChanged:{
                       passwrd = text
                       changePassword(text)
                       getPassword()    
                   }

           }
               }

               Button{

                   id:mybtn
                   text: "Login"
                   onClicked:{
                       loginButton()
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


