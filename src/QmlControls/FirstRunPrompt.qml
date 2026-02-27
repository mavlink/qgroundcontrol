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
