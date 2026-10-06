import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlyView
import QGroundControl.FlightMap

// To implement a custom overlay copy this code to your own control in your custom code source. Then override the
// FlyViewCustomLayer.qml resource with your own qml. See the custom example and documentation for details.
Item {
    id: _root

    property var occluders              // FlyViewOccluders: where the upstream widgets are, use these to position your controls
    property var customOccluders: []    // Rects of your controls which cover the map, so the map keeps the vehicle out from under them. Keep them off the view center, the map recenters there
    property var mapControl             // FlyViewMap, or FlyViewGeoMapAdapter with the GeoMap engine: add GeoMap items through QGCCorePlugin::customGeoMapItems
}
