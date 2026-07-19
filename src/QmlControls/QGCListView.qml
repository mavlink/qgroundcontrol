import QtQuick

import QGroundControl
import QGroundControl.Controls

/// QGC version of ListView control that shows horizontal/vertical scroll indicators
ListView {
    id:             root
    boundsBehavior: Flickable.StopAtBounds
    clip:           true

    property color indicatorColor: qgcPal.text

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Component.onCompleted: {
        var indicatorComponent = Qt.createComponent("QGCFlickableScrollIndicator.qml")
        indicatorComponent.createObject(root, { orientation: QGCFlickableScrollIndicator.Horizontal })
        indicatorComponent = Qt.createComponent("QGCFlickableScrollIndicator.qml")
        indicatorComponent.createObject(root, { orientation: QGCFlickableScrollIndicator.Vertical })
    }
}
