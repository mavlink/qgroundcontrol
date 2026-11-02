import QtQuick

// Both map engines create this component per click. Keep their call contract,
// but provide no popup, input targets, or guided-action handlers for customers.
Item {
    visible: false
    enabled: false
    width:   0
    height:  0

    property var mapClickCoord
    property var clickRect: Qt.rect(0, 0, 0, 0)
    property var gotoIndicator: null
    property var orbitIndicator: null

    function open() { close() }
    function close() { destroy() }
}
