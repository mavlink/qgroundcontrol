import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    id:             scriptingPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        ColumnLayout {
            id: root
            width: availableWidth
            spacing: ScreenTools.defaultFontPixelHeight * 0.75

            readonly property string scriptRoot: "/APM/scripts/"
            readonly property var luaNameFilters: [ qsTr("Lua Scripts (*.lua)"), qsTr("All Files (*)") ]
            readonly property Fact scriptingEnabledFact: factController.getParameterFact(-1, "SCR_ENABLE", false)

            readonly property var filteredEntries: {
                let rawEntries = ftpController.directoryEntries.filter(function(entry) {
                    if (!entry || entry.length < 2) {
                        return false
                    }
                    var kind = entry.charAt(0)
                    if (kind === "F") {
                        return true
                    }
                    return false
                })
                let filenameEntries = []
                for (let i=0; i<rawEntries.length; i++) {
                    filenameEntries.push(rawEntries[i].slice(1).split("\t")[0])
                }
                return filenameEntries
            }

            function fullRemotePath(filename) {
                return scriptRoot + filename
            }

            function fileNameFromPath(path) {
                if (!path) {
                    return ""
                }
                // Handle both forward slashes and backslashes
                var normalizedPath = path.replace(/\\/g, "/")
                var parts = normalizedPath.split("/")
                return parts[parts.length - 1]
            }

            function refreshDirectoryList() {
                ftpController.listDirectory(scriptRoot)
            }

            Component.onCompleted: {
                if (scriptingEnabledFact && scriptingEnabledFact.rawValue) {
                    ftpController.listDirectory(root.scriptRoot)
                }
            }

            Connections {
                target: scriptingEnabledFact

                onRawValueChanged: {
                    if (scriptingEnabledFact.rawValue) {
                        ftpController.listDirectory(root.scriptRoot)
                    } else {
                        statusText.text = ""
                    }
                }
            }

            FTPController {
                id: ftpController

                onUploadComplete: (remotePath, error) => {
                    if (error.length > 0) {
                        statusText.text = error
                    } else {
                        statusText.text = qsTr("Upload succeeded: %1").arg(remotePath)
                        refreshDirectoryList()
                    }
                }

                onDownloadComplete: (filePath, error) => {
                    if (error.length > 0) {
                        statusText.text = error
                    } else {
                        statusText.text = qsTr("Download succeeded: %1").arg(filePath)
                    }
                }

                onDeleteComplete: (remotePath, error) => {
                    if (error.length > 0) {
                        statusText.text = error
                    } else {
                        statusText.text = qsTr("Delete succeeded: %1").arg(remotePath)
                        refreshDirectoryList()
                    }
                }
            }

            FactPanelController {
                id: factController
            }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            FactCheckBoxSlider {
                text: qsTr("Enable Scripting")
                fact: scriptingEnabledFact
                enabled: scriptingEnabledFact !== null
            }

            QGCLabel {
                id: statusText
                Layout.fillWidth: true
                text: ftpController.errorString
                wrapMode: Text.Wrap
            }

            Flow {
                id: directoryView
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight
                enabled: !ftpController.busy && scriptingEnabledFact && scriptingEnabledFact.rawValue

                QGCButton {
                    text: qsTr("Upload")
                    iconSource: "/res/Upload.svg"
                    enabled: !ftpController.busy
                    onClicked: {
                        uploadDialog.folder = QGroundControl.settingsManager.appSettings.missionSavePath
                        uploadDialog.openForLoad()
                    }
                }

                Repeater {
                    model: root.filteredEntries

                    Rectangle {
                        width: button.checked ? deleteIcon.x + deleteIcon.width + button.rightPadding : button.width
                        height: button.height
                        radius: button.backRadius
                        border.width: button.showBorder ? 1 : 0
                        border.color: button.background.border.color
                        color: qgcPal.buttonHighlight

                        QGCColoredImage {
                            id: downloadIcon
                            anchors.leftMargin: button.leftPadding
                            anchors.left: parent.left
                            anchors.verticalCenter: button.verticalCenter
                            width: height
                            height: button._iconHeight
                            fillMode: Image.PreserveAspectFit
                            source: "/res/Download.svg"
                            color: button.textColor
                            visible: button.checked

                            QGCMouseArea {
                                fillItem: parent
                                onClicked: {
                                    downloadDialog.defaultSuffix = "lua"
                                    downloadDialog.title = qsTr("Download %1").arg(modelData)
                                    downloadDialog.fileToDownload = modelData
                                    downloadDialog.folder = QGroundControl.settingsManager.appSettings.missionSavePath
                                    downloadDialog.openForLoad()
                                }
                            }
                        }

                        QGCButton {
                            id: button
                            anchors.left: checked ? downloadIcon.right : parent.left
                            text: modelData
                            checkable: true
                        }

                        QGCColoredImage {
                            id: deleteIcon
                            anchors.left: button.right
                            anchors.verticalCenter: button.verticalCenter
                            width: height
                            height: button._iconHeight
                            fillMode: Image.PreserveAspectFit
                            source: "/res/TrashCan.svg"
                            color: button.textColor
                            visible: button.checked

                            QGCMouseArea {
                                fillItem: parent
                                onClicked: {
                                    var confirm = qsTr("Are you sure you want to delete the script \"%1\"? This action cannot be undone.").arg(modelData)
                                    QGroundControl.showMessageDialog(scriptingPage, qsTr("Delete Lua Script"), confirm, Dialog.Ok | Dialog.Cancel, function() {
                                        var remotePath = root.fullRemotePath(modelData)
                                        if (!ftpController.deleteFile(remotePath)) {
                                            var deleteError = ftpController.errorString.length > 0 ? ftpController.errorString : qsTr("Delete failed")
                                            QGroundControl.showMessageDialog(scriptingPage, qsTr("Lua Delete"), deleteError)
                                        }
                                    })
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight * 0.5

                QGCButton {
                    text: qsTr("Cancel Operation")
                    visible: ftpController.busy
                    onClicked: ftpController.cancelActiveOperation()
                }

                Item { Layout.fillWidth: true }

                QGCLabel {
                    text: qsTr("Transferring... %1%" ).arg(Math.round(ftpController.progress * 100))
                    visible: ftpController.busy
                }
            }

            QGCFileDialog {
                id: uploadDialog
                title: qsTr("Select Lua script to upload")
                nameFilters: root.luaNameFilters

                onAcceptedForLoad: (file) => {
                    if (!file) {
                        close()
                        return
                    }
                    var fileName = root.fileNameFromPath(file)
                    if (fileName.length === 0) {
                        close()
                        return
                    }
                    if (!fileName.toLowerCase().endsWith(".lua")) {
                        fileName = fileName + ".lua"
                    }
                    var remotePath = root.fullRemotePath(fileName)
                    if (!ftpController.uploadFile(file, remotePath)) {
                        var uploadError = ftpController.errorString.length > 0 ? ftpController.errorString : qsTr("Upload failed")
                        QGroundControl.showMessageDialog(scriptingPage, qsTr("Lua Upload"), uploadError)
                    }
                    close()
                }
            }

            QGCFileDialog {
                id: downloadDialog
                title: qsTr("Save Lua Script")
                nameFilters: root.luaNameFilters
                selectFolder: true

                property string fileToDownload

                onAcceptedForLoad: (folder) => {
                    if (!folder) {
                        close()
                        return
                    }
                    if (!ftpController.downloadFile(root.fullRemotePath(fileToDownload), folder, fileToDownload)) {
                        var downloadError = ftpController.errorString.length > 0 ? ftpController.errorString : qsTr("Download failed")
                        QGroundControl.showMessageDialog(scriptingPage, qsTr("Lua Download"), downloadError)
                    }
                    close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: availableHeight
                color: qgcPal.window
                visible: scriptingEnabledFact === null

                QGCLabel {
                    anchors.centerIn: parent
                    text: qsTr("Scripting is not supported by this version of firmware.")
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
