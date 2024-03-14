/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls

SelectableControl {
    z:                      QGroundControl.zOrderWidgets
    selectionUIRightAnchor: true
    selectedControl:        QGroundControl.settingsManager.flyViewSettings.instrumentQmlFile

    property var  missionController:    _missionController
    property real extraInset:           innerControl.extraInset
    property real extraValuesWidth:     innerControl.extraValuesWidth
}
