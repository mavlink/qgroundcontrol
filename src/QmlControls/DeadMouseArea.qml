import QtQuick 2.3
import QtQuick.Controls 1.2

MouseArea {
    preventStealing:true
    hoverEnabled:   true
    onWheel:        { wheel.accepted = true; }
    onPressed:      { mouse.accepted = true; }
    onReleased:     { mouse.accepted = true; }
}
