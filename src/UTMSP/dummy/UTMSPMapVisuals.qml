import QtQuick          2.3

Item {
    // Dummy Properties
    property var    map
    property var    myGeoFenceController
    property var    currentMissionItems
    property bool   interactive:            false
    property bool   planView:               false
    property var    homePosition
    property bool   resetCheck:             false
    property var    _dummy:              myGeoFenceController.polygons

    Instantiator {
        model: _dummy

        delegate : UTMSPMapPolygonVisuals {

        }
    }

}
