import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id: onboardLogPage
    pageComponent: pageComponent
    pageDescription: qsTr("Onboard Logs allows you to download binary log files from your vehicle. Click Refresh to get list of available logs.")

    Component {
        id: pageComponent

        RowLayout {
            width: availableWidth
            height: availableHeight

            Component.onCompleted: OnboardLogController.refresh()

            QGCFlickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: gridLayout.width
                contentHeight: gridLayout.height

                GridLayout {
                    id: gridLayout
                    rows: OnboardLogController.model.count + 1
                    columns: 5
                    flow: GridLayout.TopToBottom
                    columnSpacing: ScreenTools.defaultFontPixelWidth
                    rowSpacing: 0

                    Item { } // First column is for checkboxes, so add empty item to align headers with log entries

                    Repeater {
                        model: OnboardLogController.model

                        QGCCheckBox {
                            objectName: "onboardLogCheckbox_" + index

                            Binding on checkState {
                                value: object.selected ? Qt.Checked : Qt.Unchecked
                            }

                            onClicked: object.selected = checked
                        }
                    }

                    QGCLabel { text: qsTr("Id") }

                    Repeater {
                        model: OnboardLogController.model

                        QGCLabel { text: object.id }
                    }

                    QGCLabel { text: qsTr("Date") }

                    Repeater {
                        model: OnboardLogController.model

                        QGCLabel {
                            objectName: "onboardLogDate_" + index

                            text: {
                                if (!object.received) {
                                    return ""
                                }

                                if (object.time.getUTCFullYear() < 2010) {
                                    return qsTr("Date Unknown")
                                }

                                return object.time.toLocaleString(undefined)
                            }
                        }
                    }

                    QGCLabel { text: qsTr("Size") }

                    Repeater {
                        model: OnboardLogController.model

                        QGCLabel { text: object.sizeStr }
                    }

                    QGCLabel { text: qsTr("Status") }

                    Repeater {
                        model: OnboardLogController.model

                        QGCLabel {
                            objectName: "onboardLogStatus_" + index
                            text: object.status
                        }
                    }
                }
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: false

                QGCButton {
                    objectName: "onboardLog_refreshButton"
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs
                    text: qsTr("Refresh")

                    onClicked: {
                        if (!QGroundControl.multiVehicleManager.activeVehicle || QGroundControl.multiVehicleManager.activeVehicle.isOfflineEditingVehicle) {
                            QGroundControl.showMessageDialog(onboardLogPage, qsTr("Onboard Log Refresh"), qsTr("You must be connected to a vehicle in order to download onboard logs."))
                            return
                        }

                        OnboardLogController.refresh()
                    }
                }

                QGCButton {
                    objectName: "onboardLog_selectAllButton"
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs && (OnboardLogController.model.count > 0)
                    text: OnboardLogController.allLogsSelected ? qsTr("Deselect All") : qsTr("Select All")
                    onClicked: OnboardLogController.selectAll(!OnboardLogController.allLogsSelected)
                }

                QGCButton {
                    objectName: "onboardLog_downloadButton"
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs && (OnboardLogController.selectedCount > 0)
                    text: qsTr("Download")

                    onClicked: {
                        if (ScreenTools.isMobile) {
                            OnboardLogController.download()
                            return
                        }

                        fileDialog.title = qsTr("Select save directory")
                        fileDialog.folder = QGroundControl.settingsManager.appSettings.logSavePath
                        fileDialog.selectFolder = true
                        fileDialog.openForLoad()
                    }

                    QGCFileDialog {
                        id: fileDialog
                        onAcceptedForLoad: (file) => {
                            OnboardLogController.download(file)
                            close()
                        }
                    }
                }

                QGCButton {
                    objectName: "onboardLog_sortButton"
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs && (OnboardLogController.model.count > 1)
                    text: OnboardLogController.sortAscending ? qsTr("Sort Descending") : qsTr("Sort Ascending")
                    onClicked: OnboardLogController.toggleSortByDate()
                }

                QGCButton {
                    objectName: "onboardLog_eraseSelectedButton"
                    Layout.fillWidth: true
                    visible: OnboardLogController.transport === "ftp"
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs && (OnboardLogController.selectedCount > 0)
                    text: qsTr("Erase Selected")
                    onClicked: QGroundControl.showMessageDialog(
                        onboardLogPage,
                        qsTr("Delete Selected Onboard Log Files"),
                        qsTr("The selected onboard log files will be erased permanently. Is this really what you want?"),
                        Dialog.Yes | Dialog.No,
                        function() { OnboardLogController.eraseSelected() }
                    )
                }

                QGCButton {
                    objectName: "onboardLog_eraseAllButton"
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs && (OnboardLogController.model.count > 0)
                    text: qsTr("Erase All")
                    onClicked: QGroundControl.showMessageDialog(
                        onboardLogPage,
                        qsTr("Delete All Onboard Log Files"),
                        qsTr("All onboard log files will be erased permanently. Is this really what you want?"),
                        Dialog.Yes | Dialog.No,
                        function() { OnboardLogController.eraseAll() }
                    )
                }

                QGCButton {
                    objectName: "onboardLog_cancelButton"
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    enabled: OnboardLogController.requestingList || OnboardLogController.downloadingLogs
                    onClicked: OnboardLogController.cancel()
                }
            }
        }
    }
}
