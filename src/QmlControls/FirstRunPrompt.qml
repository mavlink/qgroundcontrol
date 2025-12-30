/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls

// Base class for all first run prompt dialogs
QGCPopupDialog {
    buttons: Dialog.Ok

    property int  promptId
    property bool markAsShownOnClose: true

    onClosed: {
        if (markAsShownOnClose) {
            QGroundControl.settingsManager.appSettings.firstRunPromptIdsMarkIdAsShown(promptId)
        }
    }
}
