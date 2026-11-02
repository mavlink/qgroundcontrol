import QtQuick

// Hide the toolbar display only; vehicle ESC telemetry is unchanged.
Item {
    readonly property bool showIndicator: false
    visible: false
    enabled: false
    width: 0
    height: 0
}
