import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.2
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0

/// This control is meant to be a direct replacement for the standard Qml FileDialog control.
/// It differs for mobile builds which uses a completely custom file picker.
Item {
    id:         _root
    visible:    false

    property string folder              // Due to Qt bug with file url parsing this must be an absolute path
    property var    nameFilters:    []  // Important: Only name filters with simple wildcarding like *.foo are supported.
    property string title
    property bool   selectExisting: true
    property bool   selectFolder:   false

    signal acceptedForLoad(string file)
    signal acceptedForSave(string file)
    signal rejected

    function openForLoad() {
        _openForLoad = true
        if (_mobileDlg && folder.length !== 0) {
            mainWindow.showComponentDialog(mobileFileOpenDialog, title, mainWindow.showDialogDefaultWidth, StandardButton.Cancel)
        } else {
            fullFileDialog.open()
        }
    }

    function openForSave() {
        _openForLoad = false
        if (_mobileDlg && folder.length !== 0) {
            mainWindow.showComponentDialog(mobileFileSaveDialog, title, mainWindow.showDialogDefaultWidth, StandardButton.Cancel | StandardButton.Ok)
        } else {
            fullFileDialog.open()
        }
    }

    function close() {
        fullFileDialog.close()
    }

    property bool   _openForLoad:   true
    property real   _margins:       ScreenTools.defaultFontPixelHeight / 2
    property bool   _mobileDlg:     QGroundControl.corePlugin.options.useMobileFileDialog
    property var    _rgExtensions
    property string _mobileShortPath

    Component.onCompleted: {
        _setupFileExtensions()
        _updateMobileShortPath()
    }

    onFolderChanged:        _updateMobileShortPath()
    onNameFiltersChanged:   _setupFileExtensions()

    function _updateMobileShortPath() {
        if (ScreenTools.isMobile) {
            _mobileShortPath = controller.fullFolderPathToShortMobilePath(folder);
        }
    }

    function _setupFileExtensions() {
        _rgExtensions = [ ]
        for (var i=0; i<_root.nameFilters.length; i++) {
            var filter = _root.nameFilters[i]
            var regExp = /^.*\((.*)\)$/
            var result = regExp.exec(filter)
            if (result.length === 2) {
                filter = result[1]
            }
            var rgFilters = filter.split(" ")
            for (var j=0; j<rgFilters.length; j++) {
                if (!_mobileDlg || (rgFilters[j] !== "*" && rgFilters[j] !== "*.*")) {
                    _rgExtensions.push(rgFilters[j])
                }
            }
        }
    }

    QGCFileDialogController { id: controller }
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    FileDialog {
        id:             fullFileDialog
        folder:         "file:///" + _root.folder
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

                    QGCLabel { text: qsTr("Path: %1").arg(_mobileShortPath) }

                    Repeater {
                        id:     fileRepeater
                        model:  controller.getFiles(folder, _rgExtensions)

                        FileButton {
                            id:             fileButton
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           modelData

                            onClicked: {
                                hideDialog()
                                _root.acceptedForLoad(controller.fullyQualifiedFilename(folder, modelData))
                            }

                            onHamburgerClicked: {
                                highlight = true
                                hamburgerMenu.fileToDelete = controller.fullyQualifiedFilename(folder, modelData)
                                hamburgerMenu.popup()
                            }

                            QGCMenu {
                                id: hamburgerMenu

                                property string fileToDelete

                                onAboutToHide: fileButton.highlight = false

                                QGCMenuItem {
                                    text:           qsTr("Delete")
                                    onTriggered: {
                                        controller.deleteFile(hamburgerMenu.fileToDelete)
                                        fileRepeater.model = controller.getFiles(folder, _rgExtensions)
                                    }
                                }
                            }
                        }
                    }

                    QGCLabel {
                        text:       qsTr("No files")
                        visible:    fileRepeater.model.length === 0
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
                    if (controller.fileExists(controller.fullyQualifiedFilename(folder, filenameTextField.text, _rgExtensions))) {
                        replaceMessage.visible = true
                        return
                    }
                }
                _root.acceptedForSave(controller.fullyQualifiedFilename(folder, filenameTextField.text, _rgExtensions))
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
                        id:     fileRepeater
                        model:  controller.getFiles(folder, [ fileExtension ])

                        FileButton {
                            id:             fileButton
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            text:           modelData

                            onClicked: {
                                hideDialog()
                                _root.acceptedForSave(controller.fullyQualifiedFilename(folder, modelData))
                            }

                            onHamburgerClicked: {
                                highlight = true
                                hamburgerMenu.fileToDelete = controller.fullyQualifiedFilename(folder, modelData)
                                hamburgerMenu.popup()
                            }

                            QGCMenu {
                                id: hamburgerMenu

                                property string fileToDelete

                                onAboutToHide: fileButton.highlight = false

                                QGCMenuItem {
                                    text:           qsTr("Delete")
                                    onTriggered: {
                                        controller.deleteFile(hamburgerMenu.fileToDelete)
                                        fileRepeater.model = controller.getFiles(folder, [ fileExtension ])
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
