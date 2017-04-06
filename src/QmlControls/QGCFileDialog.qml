import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0

/// This control is meant to be a direct replacement for the standard Qml FileDialog control.
/// It differs for mobile builds which uses a completely custom file picker.
Item {
    id:         _root
    visible:    false

    property var    qgcView
    property string folder
    property var    nameFilters
    property string fileExtension
    property string title
    property bool   selectExisting
    property bool   selectFolder

    property bool _openForLoad
    property real _margins: ScreenTools.defaultFontPixelHeight / 2

    function openForLoad() {
        _openForLoad = true
        if (ScreenTools.isMobile && folder.length !== 0) {
            qgcView.showDialog(mobileFileOpenDialog, title, qgcView.showDialogDefaultWidth, StandardButton.Cancel)
        } else {
            fullFileDialog.open()
        }
    }

    function openForSave() {
        _openForLoad = false
        if (ScreenTools.isMobile && folder.length !== 0) {
            qgcView.showDialog(mobileFileSaveDialog, title, qgcView.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
        } else {
            fullFileDialog.open()
        }
    }

    function close() {
        fullFileDialog.close()
    }

    signal acceptedForLoad(string file)
    signal acceptedForSave(string file)
    signal rejected

    QFileDialogController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    FileDialog {
        id:             fullFileDialog
        folder:         "file://" + _root.folder
        nameFilters:    _root.nameFilters ? _root.nameFilters : []
        title:          _root.title
        selectExisting: _root.selectExisting
        selectMultiple: false
        selectFolder:   _root.selectFolder

        onAccepted: {
            if (_openForLoad) {
                _root.acceptedForLoad(controller.urlToLocalFile(fileUrl))
            } else {
                _root.acceptedForSave(controller.urlToLocalFile(fileUrl))
            }
        }
        onRejected: _root.rejected()
    }

    Component {
        id: mobileFileOpenDialog

        QGCViewDialog {
            Item {
                anchors.margins:    _margins
                anchors.fill:       parent

                QGCListView {
                    id:             listView
                    anchors.fill:   parent
                    spacing:        _margins / 2
                    orientation:    ListView.Vertical
                    model:          controller.getFiles(folder, fileExtension)

                    delegate: QGCButton {
                        text: modelData

                        onClicked: {
                            hideDialog()
                            _root.acceptedForLoad(controller.fullyQualifiedFilename(folder, modelData, fileExtension))
                        }
                    }
                }

                QGCLabel {
                    text:       qsTr("No files")
                    visible:    listView.model.length == 0
                }
            }
        }
    }

    Component {
        id: mobileFileSaveDialog

        QGCViewDialog {
            function accept() {
                if (filenameTextField.text == "") {
                    return
                }
                if (!replaceMessage.visible) {
                    if (controller.fileExists(controller.fullyQualifiedFilename(folder, filenameTextField.text, fileExtension))) {
                        replaceMessage.visible = true
                        return
                    }
                }
                _root.acceptedForSave(controller.fullyQualifiedFilename(folder, filenameTextField.text, fileExtension))
                hideDialog()
            }

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
    }
}
