pragma Singleton
import QtQuick

QtObject {
    property bool loginState: true
    property bool registerButtonState: true
    property bool removeFlightPlanState: false
    property bool showActivationTab: false
    property bool enableMissionUploadButton: false
    property string startTimeStamp: ""
    property string flightID: ""
}
