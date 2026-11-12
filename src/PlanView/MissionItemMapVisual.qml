import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// Mission item map visual
Item {
    id: _root

    property var map        ///< Map control to place item in
    property var vehicle    ///< Vehicle associated with this item
    property bool interactive: true    ///< Vehicle associated with this item
    property MissionItemIndicatorGroup indicatorGroup

    signal clicked(int sequenceNumber)

    Loader {
        id: mapVisualLoader

        asynchronous: true

        Component.onCompleted: {
            const properties = {
                map: _root.map,
                vehicle: _root.vehicle,
                opacity: Qt.binding(() => _root.opacity),
                interactive: Qt.binding(() => _root.interactive)
            }
            if (object.isSimpleItem) {
                properties.indicatorGroup = _root.indicatorGroup
            }
            mapVisualLoader.setSource(object.mapVisualQML, properties)
        }

        // A removed item is deleted later. A source that finishes loading before that would be created in the
        // Loader's context, which is already invalid, and Qt warns about it. Only a source that still loads is
        // stopped: a visual that is being created is cancelled with its Loader, and stopping it here makes its
        // bindings fail.
        Component.onDestruction: {
            if (sourceComponent && sourceComponent.status === Component.Loading) {
                active = false
            }
        }

        onLoaded: {
            if (!item) {
                return
            }

            item.parent = map

            if (item.clicked) {
                item.clicked.connect(_root.clicked)
            }
        }
    }
}
