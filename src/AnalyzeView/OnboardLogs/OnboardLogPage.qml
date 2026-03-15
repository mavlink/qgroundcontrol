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

                    QGCCheckBox {
                        id: headerCheckBox
                        enabled: false
                    }

                    Repeater {
                        model: OnboardLogController.model

                        QGCCheckBox {
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

                        QGCLabel { text: object.status }
                    }
                }
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: false

                QGCButton {
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
                    Layout.fillWidth: true
                    enabled: !OnboardLogController.requestingList && !OnboardLogController.downloadingLogs
                    text: qsTr("Download")

                    onClicked: {
                        var logsSelected = false
                        for (var i = 0; i < OnboardLogController.model.count; i++) {
                            if (OnboardLogController.model.get(i).selected) {
                                logsSelected = true
                                break
                            }
                        }

                        if (!logsSelected) {
                            QGroundControl.showMessageDialog(onboardLogPage, qsTr("Onboard Log"), qsTr("You must select at least one onboard log file to download."))
                            return
                        }

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
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    enabled: OnboardLogController.requestingList || OnboardLogController.downloadingLogs
                    onClicked: OnboardLogController.cancel()
                }
            }
        }
    }
}
