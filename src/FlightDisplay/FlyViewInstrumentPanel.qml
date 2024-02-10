/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

// This control contains the instruments as well and the instrument pages which include values, camera, ...
ColumnLayout {
    id:         _root
    spacing:    _toolsMargin
    z:          QGroundControl.zOrderWidgets

    property real availableHeight

    SelectableControl {
        selectionUIRightAnchor: true
        selectedControl:        QGroundControl.settingsManager.flyViewSettings.instrumentQmlFile

        property var missionController: _missionController
    }

    TerrainProgress {
        Layout.fillWidth: true
    }
}
