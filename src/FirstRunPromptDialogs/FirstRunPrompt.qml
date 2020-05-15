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

import QGroundControl           1.0
import QGroundControl.Controls  1.0

// Base class for all first run prompt dialogs
QGCPopupDialog {
    buttons:    StandardButton.Ok

    property int  promptId
    property bool markAsShownOnClose: true

    onHideDialog: {
        if (markAsShownOnClose) {
            QGroundControl.settingsManager.appSettings.firstRunPromptIdsMarkIdAsShown(promptId)
        }
    }
}
