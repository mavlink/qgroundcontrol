pragma Singleton
import QtQuick

QtObject {
    property bool loginState: true
    property bool registerButtonState: true
    property bool removeFlightPlanState: false
    property bool showActivationTab: false
    property bool enableMissionUploadButton: false
    property bool indicatorIdleStatus: true
    property bool indicatorApprovedStatus: false
    property bool indicatorActivatedStatus: false
    property bool indicatorOnMissionStatus: false
    property bool indicatorOnMissionCompleteStatus: false
    property bool indicatorDisplayStatus: false
    property int currentStateIndex: 0
    property int currentNotificationIndex: 0
    property string indicatorActivationTime: ""
    property string startTimeStamp: ""
    property string flightID: " - "
    property string serialNumber: " - "
}
