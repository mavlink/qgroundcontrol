/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.12
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.2
import QtQuick.Controls 2.5
import QtQml            2.12

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl               1.0

/// Value page for InstrumentPanel PageView
VerticalFactValueGrid {
    id:                     _root
    width:                  pageWidth
    userSettingsGroup:      valuePageUserSettingsGroup
    defaultSettingsGroup:   valuePageDefaultSettingsGroup

    property bool showSettingsIcon: true
    property bool showLockIcon:     true

    function showSettings(settingsUnlocked) {
        _root.settingsUnlocked = settingsUnlocked
    }
}
