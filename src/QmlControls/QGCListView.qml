import QtQuick 2.3

import QGroundControl.Palette 1.0

/// QGC version of ListVIew control that shows horizontal/vertial scroll indicators
ListView {
    id:             root
    boundsBehavior: Flickable.StopAtBounds

    property color indicatorColor: qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Component.onCompleted: {
        var indicatorComponent = Qt.createComponent("QGCFlickableVerticalIndicator.qml")
        indicatorComponent.createObject(root)
        indicatorComponent = Qt.createComponent("QGCFlickableHorizontalIndicator.qml")
        indicatorComponent.createObject(root)
    }
}
