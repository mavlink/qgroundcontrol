import QtQuick                  2.3
import QtQuick.Controls         1.2

/// This control is used to create a Rectangle control which does not allow mouse events to bleed through to the control
/// which is beneath it.
Rectangle {
    MouseArea {
        anchors.fill:       parent
        preventStealing:    true
        onWheel:            { wheel.accepted = true; }
        onPressed:          { mouse.accepted = true; }
        onReleased:         { mouse.accepted = true; }
    }
}
