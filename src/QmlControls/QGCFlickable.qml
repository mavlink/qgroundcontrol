import QtQuick 2.3

import QGroundControl.Palette 1.0

/// QGC version of Flickable control that shows horizontal/vertial scroll indicators
Flickable {
    id:             root
    boundsBehavior: Flickable.StopAtBounds
    clip:           true

    property color indicatorColor: qgcPal.text

    Component.onCompleted: {
        var indicatorComponent = Qt.createComponent("QGCFlickableVerticalIndicator.qml")
        indicatorComponent.createObject(root)
        indicatorComponent = Qt.createComponent("QGCFlickableHorizontalIndicator.qml")
        indicatorComponent.createObject(root)
    }
}
