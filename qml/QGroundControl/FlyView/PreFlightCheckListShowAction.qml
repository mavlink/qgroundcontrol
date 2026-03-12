import QGroundControl
import QGroundControl.Controls

ToolStripAction {
    text:           qsTr("Checklist")
    iconSource:     "/qmlimages/check.svg"
    visible:        _useChecklist
    enabled:        _useChecklist && _activeVehicle && !_activeVehicle.armed

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property bool _useChecklist:    QGroundControl.settingsManager.appSettings.useChecklist.rawValue && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
}
