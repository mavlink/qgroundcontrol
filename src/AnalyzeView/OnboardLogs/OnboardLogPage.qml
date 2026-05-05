pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.AnalyzeView
import QGroundControl.Controls
import QGroundControl.FactControls

AnalyzePage {
    id: root

    pageComponent: pageComponent
    pageDescription: OnboardLogController.transportKind === OnboardLogController.MavlinkFtp ? qsTr("Onboard Logs lists log files on the vehicle's SD card via MAVLink FTP. Click Refresh to query the vehicle.") : qsTr("Onboard Logs allows you to download binary log files from your vehicle. Click Refresh to get list of available logs.")

    Component {
        id: pageComponent

        GridLayout {
            id: pageLayout

            readonly property bool narrow: width < ScreenTools.defaultFontPixelWidth * 80

            columns: narrow ? 1 : 2
            height: root.availableHeight
            rowSpacing: ScreenTools.defaultFontPixelHeight
            width: root.availableWidth

            Component.onCompleted: OnboardLogController.ensureLoaded()

            Connections {
                target: QGroundControl.multiVehicleManager

                function onActiveVehicleChanged() {
                    OnboardLogController.ensureLoaded();
                }
            }

            Connections {
                target: OnboardLogController

                function onTransportKindChanged() {
                    OnboardLogController.ensureLoaded();
                }
            }

            OnboardLogTable {
                controller: OnboardLogController

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumHeight: pageLayout.narrow ? root.availableHeight * 0.5 : 0
            }

            ColumnLayout {
                id: controlPanel

                spacing: ScreenTools.defaultFontPixelWidth
                Layout.alignment: Qt.AlignTop
                Layout.fillHeight: !pageLayout.narrow
                Layout.fillWidth: pageLayout.narrow
                Layout.maximumHeight: pageLayout.narrow ? root.availableHeight * 0.45 : root.availableHeight
                Layout.maximumWidth: pageLayout.narrow ? root.availableWidth : root.availableWidth * 0.4
                Layout.preferredHeight: pageLayout.narrow
                                        ? Math.min(controlColumn.implicitHeight + cancelButton.implicitHeight + controlPanel.spacing,
                                                   root.availableHeight * 0.45)
                                        : root.availableHeight
                Layout.preferredWidth: pageLayout.narrow
                                       ? root.availableWidth
                                       : Math.min(ScreenTools.defaultFontPixelWidth * 30, root.availableWidth * 0.4)

                ScrollView {
                    id: controlScroll

                    contentWidth: availableWidth

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: controlColumn

                        spacing: ScreenTools.defaultFontPixelWidth
                        width: controlScroll.availableWidth

                        LabelledFactComboBox {
                            enabled: !OnboardLogController.busy
                            fact: QGroundControl.settingsManager.mavlinkSettings.onboardLogTransport
                            label: qsTr("Transport")

                            Layout.fillWidth: true
                        }

                        ProgressBar {
                            from: 0
                            to: 1
                            value: OnboardLogController.batchProgress
                            visible: OnboardLogController.downloadingLogs

                            Layout.fillWidth: true

                            Accessible.name: qsTr("Onboard log download progress")
                            Accessible.description: qsTr("%1% complete").arg(Math.round(value * 100))
                        }

                        QGCLabel {
                            horizontalAlignment: Text.AlignHCenter
                            text: qsTr("%1% complete").arg(Math.round(OnboardLogController.batchProgress * 100))
                            textFormat: Text.PlainText
                            visible: OnboardLogController.downloadingLogs

                            Layout.fillWidth: true
                        }

                        QGCLabel {
                            color: qgcPal.warningText
                            text: OnboardLogController.errorMessage
                            textFormat: Text.PlainText
                            visible: OnboardLogController.errorMessage.length > 0
                            wrapMode: Text.Wrap

                            Layout.fillWidth: true
                        }

                        QGCButton {
                            enabled: !OnboardLogController.busy
                            text: qsTr("Refresh")

                            Layout.fillWidth: true

                            onClicked: {
                                if (!QGroundControl.multiVehicleManager.activeVehicle || QGroundControl.multiVehicleManager.activeVehicle.isOfflineEditingVehicle) {
                                    QGroundControl.showMessageDialog(root, qsTr("Onboard Log Refresh"), qsTr("You must be connected to a vehicle in order to download onboard logs."));
                                    return;
                                }

                                OnboardLogController.refresh();
                            }
                        }

                        QGCButton {
                            enabled: !OnboardLogController.busy && OnboardLogController.model && OnboardLogController.model.count > 0
                            text: OnboardLogController.allLogsSelected ? qsTr("Deselect All") : qsTr("Select All")

                            Layout.fillWidth: true

                            onClicked: OnboardLogController.selectAll(!OnboardLogController.allLogsSelected)
                        }

                        QGCButton {
                            enabled: !OnboardLogController.busy
                            text: qsTr("Download")

                            Layout.fillWidth: true

                            onClicked: {
                                if (OnboardLogController.selectedCount() === 0) {
                                    QGroundControl.showMessageDialog(root, qsTr("Onboard Log"), qsTr("You must select at least one onboard log file to download."));
                                    return;
                                }

                                if (ScreenTools.isMobile) {
                                    OnboardLogController.download();
                                    return;
                                }

                                fileDialog.title = qsTr("Select save directory");
                                fileDialog.folder = QGroundControl.settingsManager.appSettings.logSavePath;
                                fileDialog.selectFolder = true;
                                fileDialog.openForLoad();
                            }

                            QGCFileDialog {
                                id: fileDialog

                                onAcceptedForLoad: file => {
                                    OnboardLogController.download(file);
                                    close();
                                }
                            }
                        }

                        QGCButton {
                            enabled: !OnboardLogController.busy && OnboardLogController.model && OnboardLogController.model.count > 1
                            text: OnboardLogController.sortAscending ? qsTr("Sort Descending") : qsTr("Sort Ascending")

                            Layout.fillWidth: true

                            onClicked: OnboardLogController.toggleSortByDate()
                        }

                        QGCButton {
                            enabled: !OnboardLogController.busy && OnboardLogController.model && OnboardLogController.model.count > 0
                            text: qsTr("Erase All")

                            Layout.fillWidth: true

                            onClicked: {
                                const eraseVehicle = QGroundControl.multiVehicleManager.activeVehicle;
                                QGroundControl.showMessageDialog(root, qsTr("Delete All Onboard Log Files"), qsTr("All onboard log files will be erased permanently. Is this really what you want?"), Dialog.Yes | Dialog.No, function () {
                                    if (!OnboardLogController.eraseAllForVehicle(eraseVehicle)) {
                                        QGroundControl.showMessageDialog(root, qsTr("Onboard Log Erase"), qsTr("The erase request was canceled because the active vehicle or onboard log state changed."));
                                    }
                                });
                            }
                        }
                    }
                }

                QGCButton {
                    id: cancelButton

                    enabled: OnboardLogController.busy
                    text: qsTr("Cancel")

                    Layout.fillWidth: true

                    onClicked: OnboardLogController.cancel()
                }
            }

            QGCPalette {
                id: qgcPal

                colorGroupEnabled: root.enabled
            }
        }
    }
}
