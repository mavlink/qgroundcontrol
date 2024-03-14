/****************************************************************************
 *
 * (c) 2009-2021 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controllers
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

ColumnLayout {
    spacing: ScreenTools.defaultFontPixelHeight

    property var  _activeVehicle:   globals.activeVehicle
    property var  _autotuneManager: _activeVehicle.autotune
    property real _margins:         ScreenTools.defaultFontPixelHeight

    QGCButton {
        id:        autotuneButton
        primary:   true
        text:      qsTr("Start AutoTune")
        enabled:   _activeVehicle.flying && !_activeVehicle.landing && !_autotuneManager.autotuneInProgress

        onClicked: mainWindow.showMessageDialog(autotuneButton.text,
                                                qsTr("WARNING!\
        \n\nThe auto-tuning procedure should be executed with caution and requires the vehicle to fly stable enough before attempting the procedure! \
        \n\nBefore starting the auto-tuning process, make sure that: \
        \n1. You have read the auto-tuning guide and have followed the preliminary steps \
        \n2. The current control gains are good enough to stabilize the drone in presence of medium disturbances \
        \n3. You are ready to abort the auto-tuning sequence by moving the RC sticks, if anything unexpected happens. \
        \n\nClick Ok to start the auto-tuning process.\n"),
                                                Dialog.Ok | Dialog.Cancel,
                                                function() { _autotuneManager.autotuneRequest() })
    }

    QGCLabel { text: _autotuneManager.autotuneStatus }

    ProgressBar {
        value: _autotuneManager.autotuneProgress
    }
}
