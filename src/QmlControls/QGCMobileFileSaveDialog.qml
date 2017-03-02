/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
    property string fileExtension   ///< File extension for file listing

    signal filenameReturned(string filename)

    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    function accept() {
        if (filenameTextField.text == "") {
            return
        }
        if (!replaceMessage.visible) {
            if (controller.fileExists(filenameTextField.text, fileExtension)) {
                console.log("File exists")
                replaceMessage.visible = true
                return
            }
        }
        filenameReturned(controller.fullPath(filenameTextField.text, fileExtension))
        hideDialog()
    }

    QGCMobileFileDialogController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        ScreenTools.defaultFontPixelHeight

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
            text:           qsTr("The file %1 exists. Click Save again to replace it.").arg(filenameTextField.text)
            visible:        false
            color:          qgcPal.warningText
        }
    }
}

