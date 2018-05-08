import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

QGCButton {
    property string name:           ""
    property int    group:          0
    property string defaulttext:    "Not checked yet"
    property string pendingtext:    ""
    property string failuretext:    "Failure. Check console."
    property int    _state:         0
    property var    _color:         qgcPal.button
    property int    _nrClicked:     0
    property string _text:          qsTr(name)+ ": " + qsTr(defaulttext)

    enabled : (_activeVehicle==null || _activeVehicle.connectionLost) ? false : checklist._checkState>=group
    opacity : (_activeVehicle==null || _activeVehicle.connectionLost) ? 0.4 : 0.2+0.8*(checklist._checkState >= group);
    width: 40*ScreenTools.defaultFontPixelWidth
    style: ButtonStyle {
        background: Rectangle {color:_color; border.color: qgcPal.button; radius:3}
        label: Label {
            text: _text
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            color: _state>0 ? qgcPal.mapWidgetBorderLight : qgcPal.buttonText
        }
    }

    // Connections
    onPendingtextChanged: { if(_state==1) {getTextFromState(); getColorFromState();} }
    onFailuretextChanged: { if(_state==3) {getTextFromState(); getColorFromState();} }
    on_StateChanged: { getTextFromState(); getColorFromState(); }
    onClicked: {
        if(_state<2) _nrClicked=_nrClicked+1; //Only allow click-counter to increase when not failed yet
        updateItem();
    }

    //Functions
    function updateItem() {
        // This is the default updateFunction. It assumes the item is a MANUAL check list item, i.e. one that
        // only requires user clicks (one click if pendingtext="", two clicks otherwise) for completion.

        if(_nrClicked===0) _state = 0;
        else if(_nrClicked===1) {
            if(pendingtext.length === 0) _state = 4;
            else _state = 1;
        } else _state = 4;

        getTextFromState();
        getColorFromState();
    }
    function getTextFromState() {
        if(_state === 0) {_text= qsTr(name) + ": " + qsTr(defaulttext)}                         // Not checked yet
        else if(_state === 1) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr(pendingtext)}        // Pending
        else if(_state === 2) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr("Minor problem")}    // Small problem or need further user action to resolve
        else if(_state === 3) {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr(failuretext)}        // Big problem
        else {_text= "<b>"+qsTr(name)+"</b>" +": " + qsTr("OK")}                                // All OK
    }
    function getColorFromState() {
        if(_state === 0) {_color=qgcPal.button}                     // Not checked yet
        else if(_state === 1) {_color=Qt.rgba(0.9,0.47,0.2,1)}      // Pending
        else if(_state === 2) {_color=Qt.rgba(1.0,0.6,0.2,1)}       // Small problem or need further user action to resolve
        else if(_state === 3) {_color=Qt.rgba(0.92,0.22,0.22,1)}    // Big problem
        else {_color=Qt.rgba(0.27,0.67,0.42,1)}                     // All OK
    }
    function resetNrClicks() {
        _nrClicked=0;
        updateItem();
    }
}
