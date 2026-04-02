import QtQuick

Item {
    id: viewer3DBody

    property bool isOpen: false

    function open() {
        isOpen = false
    }

    function close() {
        isOpen = false
    }

    visible: false
    enabled: false
}
