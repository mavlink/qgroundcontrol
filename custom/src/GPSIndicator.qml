import QtQuick

// Suppress the toolbar's GNSS/RTK UI without changing GPS processing.
Item {
    objectName: "toolbar_gpsIndicator"
    readonly property bool showIndicator: false
    visible: false
    enabled: false
    width: 0
    height: 0
}
