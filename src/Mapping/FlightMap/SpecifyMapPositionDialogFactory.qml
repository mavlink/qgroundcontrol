pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl.Controls

/// Opens the dialog in which the user enters the position that a map is then centered on.
/// A map owns its factory: a dialog closes with the owner of its factory, and a drop panel that asks for the
/// dialog is closed by then.
QGCPopupDialogFactory {
    id: root

    /// FlightMap or GeoMapView
    required property var map

    dialogComponent: Component {
        EditPositionDialog {
            title: qsTr("Specify Position")
            coordinate: root.map.center
            onCoordinateChanged: root.map.center = coordinate
        }
    }
}
