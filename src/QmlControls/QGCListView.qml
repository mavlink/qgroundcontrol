import QtQuick

import QGroundControl.Palette

/// QGC version of ListVIew control that shows horizontal/vertial scroll indicators
ListView {
    id:             root
    boundsBehavior: Flickable.StopAtBounds
    clip:           true

    property color indicatorColor: qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Component.onCompleted: {
        var indicatorComponent = Qt.createComponent("QGCFlickableVerticalIndicator.qml")
        indicatorComponent.createObject(root)
        indicatorComponent = Qt.createComponent("QGCFlickableHorizontalIndicator.qml")
        indicatorComponent.createObject(root)
    }
}
