import QtQuick

Item {
    property bool displayStatus: UTMSPStateStorage.indicatorDisplayStatus
    property var overlay
    property var indicatorTopText: []
    property var indicatorBottomText: [ ]
    property int currentNotificationIndex: false
}
