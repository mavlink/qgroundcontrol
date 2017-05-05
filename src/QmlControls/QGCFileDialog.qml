import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

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
    property string fileExtension2
    property string title
    property bool   selectExisting
    property bool   selectFolder

    property bool _openForLoad: true
    property real _margins:     ScreenTools.defaultFontPixelHeight / 2
    property bool _mobile:      ScreenTools.isMobile


    function openForLoad() {
        _openForLoad = true
        if (_mobile && folder.length !== 0) {
            qgcView.showDialog(mobileFileOpenDialog, title, qgcView.showDialogDefaultWidth, StandardButton.Cancel)
        } else {
            fullFileDialog.open()
        }
    }

    function openForSave() {
        _openForLoad = false
        if (_mobile && folder.length !== 0) {
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
            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  fileOpenColumn.height

                Column {
                    id:             fileOpenColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight / 2

                    Repeater {
                        id:     fileList
                        model:  controller.getFiles(folder, fileExtension)

                        QGCButton {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           modelData

                            onClicked: {
                                hideDialog()
                                _root.acceptedForLoad(controller.fullyQualifiedFilename(folder, modelData, fileExtension))
                            }
                        }
                    }

                    Repeater {
                        id:     fileList2
                        model:  fileExtension2 == "" ? [ ] : controller.getFiles(folder, fileExtension2)

                        QGCButton {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           modelData

                            onClicked: {
                                hideDialog()
                                _root.acceptedForLoad(controller.fullyQualifiedFilename(folder, modelData, fileExtension2))
                            }
                        }
                    }

                    QGCLabel {
                        text:       qsTr("No files")
                        visible:    fileList.model.length == 0 && fileList2.model.length == 0
                    }
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

            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  fileSaveColumn.height

                Column {
                    id:             fileSaveColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        ScreenTools.defaultFontPixelHeight / 2

                    RowLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        ScreenTools.defaultFontPixelWidth

                        QGCLabel { text: qsTr("New file name:") }

                        QGCTextField {
                            id:                 filenameTextField
                            Layout.fillWidth:   true
                            onTextChanged:      replaceMessage.visible = false
                        }
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

                    SectionHeader {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           qsTr("Save to existing file:")
                    }

                    Repeater {
                        model: controller.getFiles(folder, fileExtension)

                        QGCButton {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           modelData

                            onClicked: {
                                hideDialog()
                                _root.acceptedForSave(controller.fullyQualifiedFilename(folder, modelData, fileExtension))
                            }
                        }
                    }


                }
            }
        }
    }
}
