import QtQuick

import QGroundControl
import QGroundControl.Controls

QGCLabel {
    id: control
    objectName: "mainStatusText"
    enabled: false
    focusPolicy: Qt.NoFocus
    verticalAlignment: Text.AlignVCenter
    font.pointSize: ScreenTools.largeFontPointSize
    color: qgcPal.text
    text: _connected ? qsTr("Connected") : qsTr("Disconnected")

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _connected: !!_activeVehicle && !_activeVehicle.vehicleLinkManager.communicationLost

    // Keep callers compatible without exposing a drawer or configuration menu.
    function dropMainStatusIndicator(showMessages = false) {
    }

    QGCPalette { id: qgcPal }

    // Preserve configured checklist tracking even though its UI is not exposed.
    Loader {
        visible: false
        active: QGroundControl.settingsManager.appSettings.useChecklist.rawValue && !!control._activeVehicle
        source: "qrc:/qml/QGroundControl/FlyView/PreFlightCheckList.qml"
        onLoaded: {
            if (control._activeVehicle) {
                control._activeVehicle.checkListState = item.allChecksPassed === true
                    ? Vehicle.CheckListPassed : Vehicle.CheckListNotSetup
            }
        }
    }
}
