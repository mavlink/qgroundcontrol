/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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
