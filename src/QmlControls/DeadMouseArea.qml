import QtQuick
import QtQuick.Controls

MouseArea {
    preventStealing:true
    hoverEnabled:   true
    onWheel:        { wheel.accepted = true; }
    onPressed:      { mouse.accepted = true; }
    onReleased:     { mouse.accepted = true; }
}
