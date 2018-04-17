import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

QGCButton {
    property string name: ""
    property int _state: 0
    property var _color: qgcPal.button;//qgcPal.windowShade;//qgcPal.windowShadeDark;//Qt.rgba(0.5,0.5,0.5,1) //qgcPal.window;//
    property int _nrClicked: 0
    property int group: 0
    property string defaulttext: "Not checked yet"
    property string pendingtext: ""
    property string failuretext: "Failure. Check console."
    property string _text: qsTr(name)+ ": " + qsTr(defaulttext)

    enabled : (_activeVehicle==null || _activeVehicle.connectionLost) ? false : _checkState>=group
    opacity : (_activeVehicle==null || _activeVehicle.connectionLost) ? 0.4 : 0.2+0.8*(_checkState >= group);

    width: parent.width
    style: ButtonStyle {
        background: Rectangle {color:_color; border.color: qgcPal.button; radius:3}
        label: Label {
            text: _text
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: _state>0 ? qgcPal.mapWidgetBorderLight : qgcPal.buttonText
        }
    }

    onClicked: {
        if(_state<2) _nrClicked=_nrClicked+1; //Only allow click-counter to increase when not failed yet
        updateItem();
    }
    onPendingtextChanged: { if(_state==1) {getTextFromState(); getColorFromState();} }
    onFailuretextChanged: { if(_state==3) {getTextFromState(); getColorFromState();} }
    on_StateChanged: { getTextFromState(); getColorFromState(); }
//    onEnabledChanged: { //Dont do this for now, because if we only accidentially lose connection, we don't want to delete the checklist state. Instead, we'd need to detect a re-connect, maybe based on the timesincesystemstart (i.e. when it decreases)?
//        if(enabled==false && group > 0) {
//            // Reset all check list items of group > 0 if it is disabled again (which e.g. happens after a vehicle reboot or disarm).
//            _nrClicked = 0;
//            _state = 0;
//        }
//    }

    function updateItem() {
        // This is the default updateFunction. It assumes the item is a MANUAL check list item, i.e. one that
        // only requires user clicks (one click if pendingtext="", two clicks otherwise) for completion.
        //if(_nrClicked>0) _state = 4;

        if(_nrClicked===1) {
            if(pendingtext.length === 0) _state = 4;
            else _state = 1;
        } else if(_nrClicked>1) _state = 4;

        getTextFromState();
        getColorFromState();
    }
    function getTextFromState() {
        if(_state === 0) {_text= qsTr(name) + ": " + qsTr(defaulttext)}             // Not checked yet
        else if(_state === 1) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr(pendingtext)}         // Pending
        else if(_state === 2) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr("Minor problem")}     // Small problem or need further user action to resolve
        else if(_state === 3) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr(failuretext)}         // Big problem
        else {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr("OK")}                                // All OK
    }
    function getColorFromState() {
        if(_state === 0) {_color=qgcPal.button}            // Not checked yet
        else if(_state === 1) {_color=Qt.rgba(0.9,0.47,0.2,1)}      // Pending
        else if(_state === 2) {_color=Qt.rgba(1.0,0.6,0.2,1)}       // Small problem or need further user action to resolve
        else if(_state === 3) {_color=Qt.rgba(0.92,0.22,0.22,1)}    // Big problem
        else {_color=Qt.rgba(0.27,0.67,0.42,1)}                     // All OK
    }
}
