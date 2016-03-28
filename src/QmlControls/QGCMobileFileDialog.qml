/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0

/// Simple file picker for mobile
QGCViewDialog {
    property bool   openDialog:     true    ///< true: Show file open dialog, false: show file save dialog
    property string fileExtension           ///< File extension for file listing

    signal filenameReturned(string filename)

    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    function accept() {
        if (!openDialog) {
            if (!dialogLoader.item.replaceMessageShown) {
                if (controller.fileExists(dialogLoader.item.filename, fileExtension)) {
                    dialogLoader.item.replaceMessageShown = true
                    return
                }
            }
            filenameReturned(controller.fullPath(dialogLoader.item.filename, fileExtension))
        }
        hideDialog()
    }

    QGCMobileFileDialogController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Loader {
        id:                 dialogLoader
        anchors.fill:       parent
        sourceComponent:    openDialog ? openDialogComponent : saveDialogComponent
    }

    Component {
        id: saveDialogComponent

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        ScreenTools.defaultFontPixelHeight

            property alias filename:            filenameTextField.text
            property alias replaceMessageShown: replaceMessage.visible

            QGCLabel {
                text: qsTr("File name:")
            }

            QGCTextField {
                id:             filenameTextField
                onTextChanged:  replaceMessage.visible = false
            }

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("File names must end with .%1 file extension. If missing it will be added.").arg(fileExtension)
            }

            QGCLabel {
                id:             replaceMessage
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                text:           qsTr("The file %1 exists. Click Save again to replace it.").arg(filename)
                visible:        false
                color:          qgcPal.warningText
            }
        }
    }

    Component {
        id: openDialogComponent

        Item {
            anchors.margins:    _margins
            anchors.fill:       parent

            ListView {
                anchors.fill:   parent
                spacing:        _margins / 2
                orientation:    ListView.Vertical
                model:          controller.getFiles(fileExtension)

                delegate: QGCButton {
                    text: modelData

                    onClicked: {
                        hideDialog()
                        filenameReturned(controller.fullPath(modelData, fileExtension))
                    }
                }
            }

            QGCLabel {
                text:       qsTr("No files")
                visible:    controller.getFiles(fileExtension).length == 0
            }
        }
    }
}
