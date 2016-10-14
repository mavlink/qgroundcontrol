/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs  1.1

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    width:  parent ? parent.width : 0
    height: logColumn.height

    function saveSettings() {
        if(subEditConfig) {
            subEditConfig.filename = logField.text
        }
    }

    Column {
        id:         logColumn
        width:      parent.width
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        QGCLabel {
            text:   qsTr("Log Replay Link Settings")
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        Row {
            spacing:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   qsTr("Log File:")
                width:  _firstColumn
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCTextField {
                id:     logField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeLogReplay ? subEditConfig.fileName : ""
                width:  _secondColumn
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCButton {
                text:   qsTr("Browse")
                onClicked: {
                    fileDialog.visible = true
                }
            }
        }
        FileDialog {
            id:         fileDialog
            title:      qsTr("Please choose a file")
            folder:     shortcuts.home
            visible:    false
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
}
