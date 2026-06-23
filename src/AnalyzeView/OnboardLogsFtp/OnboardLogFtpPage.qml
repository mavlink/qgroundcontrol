import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

import QGroundControl
import QGroundControl.Controls

AnalyzePage {
    id: onboardLogFtpPage
    pageComponent: pageComponent
    pageDescription: qsTr("Onboard Logs (FTP) lists log files on the vehicle's SD card via MAVLink FTP. Click Refresh to query the vehicle.")

    Component {
        id: pageComponent

        RowLayout {
            width: availableWidth
            height: availableHeight

            QGCFlickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: gridLayout.width
                contentHeight: gridLayout.height

                GridLayout {
                    id: gridLayout
                    rows: OnboardLogFtpController.model.count + 1
                    columns: 5
                    flow: GridLayout.TopToBottom
                    columnSpacing: ScreenTools.defaultFontPixelWidth
                    rowSpacing: 0

                    QGCCheckBox {
                        id: headerCheckBox
                        enabled: false
                    }

                    Repeater {
                        model: OnboardLogFtpController.model

                        QGCCheckBox {
                            Binding on checkState {
                                value: object.selected ? Qt.Checked : Qt.Unchecked
                            }

                            onClicked: object.selected = checked
                        }
                    }

                    QGCLabel { text: qsTr("Id") }

                    Repeater {
                        model: OnboardLogFtpController.model

                        QGCLabel { text: object.id }
                    }

                    QGCLabel { text: qsTr("Date") }

                    Repeater {
                        model: OnboardLogFtpController.model

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
                        model: OnboardLogFtpController.model

                        QGCLabel { text: object.sizeStr }
                    }

                    QGCLabel { text: qsTr("Status") }

                    Repeater {
                        model: OnboardLogFtpController.model

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
                    enabled: !OnboardLogFtpController.requestingList && !OnboardLogFtpController.downloadingLogs
                    text: qsTr("Refresh")

                    onClicked: {
                        if (!QGroundControl.multiVehicleManager.activeVehicle || QGroundControl.multiVehicleManager.activeVehicle.isOfflineEditingVehicle) {
                            QGroundControl.showMessageDialog(onboardLogFtpPage, qsTr("Log Refresh"), qsTr("You must be connected to a vehicle in order to download logs."))
                            return
                        }

                        OnboardLogFtpController.refresh()
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    enabled: !OnboardLogFtpController.requestingList && !OnboardLogFtpController.downloadingLogs
                    text: qsTr("Download")

                    onClicked: {
                        var logsSelected = false
                        for (var i = 0; i < OnboardLogFtpController.model.count; i++) {
                            if (OnboardLogFtpController.model.get(i).selected) {
                                logsSelected = true
                                break
                            }
                        }

                        if (!logsSelected) {
                            QGroundControl.showMessageDialog(onboardLogFtpPage, qsTr("Log Download"), qsTr("You must select at least one log file to download."))
                            return
                        }

                        if (ScreenTools.isMobile) {
                            OnboardLogFtpController.download()
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
                            OnboardLogFtpController.download(file)
                            close()
                        }
                    }
                }

                QGCButton {
                    Layout.fillWidth: true
                    text: qsTr("Cancel")
                    enabled: OnboardLogFtpController.requestingList || OnboardLogFtpController.downloadingLogs
                    onClicked: OnboardLogFtpController.cancel()
                }
            }
        }
    }
}
