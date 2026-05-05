import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.AnalyzeView
import QGroundControl.Controls

AnalyzePage {
    id: onboardLogPage
    pageComponent: pageComponent
    pageDescription: OnboardLogController.transportName === "ftp"
        ? qsTr("Onboard Logs lists log files on the vehicle's SD card via MAVLink FTP. Click Refresh to query the vehicle.")
        : qsTr("Onboard Logs allows you to download binary log files from your vehicle. Click Refresh to get list of available logs.")

    Component {
        id: pageComponent

        RowLayout {
            width: availableWidth
            height: availableHeight

            Component.onCompleted: OnboardLogController.refresh()

            OnboardLogTable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                controller: OnboardLogController
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
                        var listModel = OnboardLogController.model
                        if (listModel) {
                            for (var i = 0; i < listModel.count; i++) {
                                if (listModel.get(i).selected) {
                                    logsSelected = true
                                    break
                                }
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
                    visible: OnboardLogController.canErase
                    enabled: OnboardLogController.canErase &&
                             !OnboardLogController.requestingList &&
                             !OnboardLogController.downloadingLogs &&
                             OnboardLogController.model && OnboardLogController.model.count > 0
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
