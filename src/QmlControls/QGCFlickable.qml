import QtQuick  2.5

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Flickable {
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
