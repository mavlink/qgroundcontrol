import QtQuick

// Keep the caller contract without reserving space or constructing guided actions.
Item {
    property real maxHeight: 0
    readonly property var model: []

    visible: false
    enabled: false
    width: 0
    height: 0

    function simulateClick(buttonIndex) { }
}
