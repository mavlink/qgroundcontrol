import QtQuick          2.3
import QGroundControl.UTMSP                 1.0

Item {
    // Dummy Properties
    property var    myGeoFenceController
    property var    flightMap
    property var    currentMissionItems
    property bool   triggerSubmitButton
    property bool   resetRegisterFlightPlan
    // Dummy Signals
     signal responseSent(string response)
     signal vehicleIDSent(int id)
     signal resetGeofencePolygonTriggered()
     signal timeStampSent(string timestamp, bool activateflag, string id)
     signal removeFlightPlanTriggered()
}
