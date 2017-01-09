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

/// Simple file open dialog for mobile
QGCViewDialog {
    property string fileExtension   ///< File extension for file listing

    signal filenameReturned(string filename)

    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    QGCMobileFileDialogController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Item {
        anchors.margins:    _margins
        anchors.fill:       parent

        QGCListView {
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
