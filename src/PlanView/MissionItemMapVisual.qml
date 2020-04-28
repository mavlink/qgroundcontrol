/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0


/// Mission item map visual
Item {
    id: _root

    property var map        ///< Map control to place item in
    property var vehicle    ///< Vehicle associated with this item
    property var interactive: true    ///< Vehicle associated with this item

    signal clicked(int sequenceNumber)

    property var _visualItem

    Component.onCompleted: {
        if (object.mapVisualQML) {
            var component = Qt.createComponent(object.mapVisualQML)
            if (component.status === Component.Error) {
                console.log("Error loading Qml: ", object.mapVisualQML, component.errorString())
            }
            _visualItem = component.createObject(map, { "map": _root.map, vehicle: _root.vehicle, 'opacity': Qt.binding(function() { return _root.opacity }), 'interactive': Qt.binding(function() { return _root.interactive }) })
            _visualItem.clicked.connect(_root.clicked)
        }
    }

    Component.onDestruction: {
        if (_visualItem) {
            _visualItem.destroy()
        }
    }
}
