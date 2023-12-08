import QtQuick
import QtQuick.Controls

MouseArea {
    preventStealing:true
    hoverEnabled:   true
    onWheel:    (wheel) => { wheel.accepted = true; }
    onPressed:  (mouse) => { mouse.accepted = true; }
    onReleased: (mouse) => { mouse.accepted = true; }
}
