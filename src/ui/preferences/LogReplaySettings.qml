/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Column {
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
    anchors.margins:    ScreenTools.defaultFontPixelWidth
    function saveSettings() {
        if(subEditConfig) {
            subEditConfig.filename = logField.text
        }
    }
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:       qsTr("Log File:")
            width:      _firstColumn
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCTextField {
            id:         logField
            text:       subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeLogReplay ? subEditConfig.fileName : ""
            width:      _secondColumn
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCButton {
            text:       qsTr("Browse")
            onClicked: {
                fileDialog.visible = true
            }
        }
    }
    FileDialog {
        id:             fileDialog
        title:          qsTr("Please choose a file")
        folder:         shortcuts.home
        visible:        false
        selectExisting: true
        onAccepted: {
            if(subEditConfig) {
                subEditConfig.fileName = fileDialog.fileUrl.toString().replace("file://", "")
            }
            fileDialog.visible = false
        }
        onRejected: {
            fileDialog.visible = false
        }
    }
}
