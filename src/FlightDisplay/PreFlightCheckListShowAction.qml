/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QGroundControl           1.0
import QGroundControl.Controls  1.0

ToolStripAction {
    text:           qsTr("Checklist")
    iconSource:     "/qmlimages/check.svg"
    visible:        _useChecklist
    enabled:        _useChecklist && _activeVehicle && !_activeVehicle.armed

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property bool _useChecklist:    QGroundControl.settingsManager.appSettings.useChecklist.rawValue && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
}
