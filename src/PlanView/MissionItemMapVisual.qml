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

        // An item that leaves the mission has this view released at once and deleted later. A visual whose QML
        // finishes loading in between would be created in the released view's context, which Qt warns of, so that
        // loading is given up. A visual that is already being created is left to finish: Qt would delete it piece
        // by piece under its own bindings.
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
