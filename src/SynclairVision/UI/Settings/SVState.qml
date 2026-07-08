pragma Singleton
import QtQuick

QtObject {
    id: root

//---------------------------------
// Overlay
//---------------------------------
    property bool synclairOverlay: true
    property bool hud: true
    property bool toolbar: true
    property bool lockControls: false
    property string layout: "single"
    property bool record: false
}