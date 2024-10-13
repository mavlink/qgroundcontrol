/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Qt.labs.qmlmodels

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

AnalyzePage {
    id:                 logDownloadPage
    pageComponent:      pageComponent
    pageDescription:    qsTr("Log Download allows you to download binary log files from your vehicle. Click Refresh to get list of available logs.")

    property real _margin: ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Component {
        id: pageComponent

        RowLayout {
            width:  availableWidth
            height: availableHeight

            QGCFlickable {
                Layout.fillWidth:   true
                Layout.fillHeight:  true
                contentWidth:       gridLayout.width
                contentHeight:      gridLayout.height

                GridLayout {
                    id:                 gridLayout
                    rows:               logController.model.count + 1
                    columns:            5
                    flow:               GridLayout.TopToBottom
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    rowSpacing:         0

                    QGCCheckBox {
                        id:         headerCheckBox
                        enabled:    false
                    }

                    Repeater {
                        model: logController.model

                        QGCCheckBox {
                            Binding on checkState {
                                value: object.selected ? Qt.Checked : Qt.Unchecked
                            }

                            onClicked: object.selected = checked
                        }
                    }

                    QGCLabel {
                        text: qsTr("Id")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel {
                            text: object.id 
                        }
                    }

                    QGCLabel {
                        text: qsTr("Date")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel {
                            text: {
                                if (object.received) {
                                    var d = object.time
                                    if (d.getUTCFullYear() < 2010)
                                        return qsTr("Date Unknown")
                                    else
                                        return d.toLocaleString(undefined)
                                }
                                return ""
                            }
                        }
                    }

                    QGCLabel { 
                        text: qsTr("Size")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel { text: object.sizeStr }
                    }

                    QGCLabel { 
                        text: qsTr("Status")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel { text: object.status }
                    }
                }
            }

            ColumnLayout {
                spacing:            _margin
                Layout.alignment:   Qt.AlignTop
                Layout.fillWidth:   false

                QGCButton {
                    Layout.fillWidth:   true
                    enabled:            !logController.requestingList && !logController.downloadingLogs
                    text:               qsTr("Refresh")

                    onClicked: {
                        if (!QGroundControl.multiVehicleManager.activeVehicle || QGroundControl.multiVehicleManager.activeVehicle.isOfflineEditingVehicle) {
                            mainWindow.showMessageDialog(qsTr("Log Refresh"), qsTr("You must be connected to a vehicle in order to download logs."))
                        } else {
                            logController.refresh()
                        }
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    enabled:            !logController.requestingList && !logController.downloadingLogs
                    text:               qsTr("Download")

                    onClicked: {
                        var logsSelected = false
                        for (var i = 0; i < logController.model.count; i++) {
                            var o = logController.model.get(i)
                            if (o.selected) {
                                logsSelected = true
                                break
                            }
                        }
                        if (!logsSelected) {
                            mainWindow.showMessageDialog(qsTr("Log Download"), qsTr("You must select at least one log file to download."))
                            return
                        }

                        if (ScreenTools.isMobile) {
                            // You can't pick folders in mobile, only default location is used
                            logController.download()
                        } else {
                            fileDialog.title =          qsTr("Select save directory")
                            fileDialog.folder =         QGroundControl.settingsManager.appSettings.logSavePath
                            fileDialog.selectFolder =   true
                            fileDialog.openForLoad()
                        }
                    }

                    QGCFileDialog {
                        id: fileDialog
                        onAcceptedForLoad: (file) => {
                            logController.download(file)
                            close()
                        }
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    enabled:            !logController.requestingList && !logController.downloadingLogs && logController.model.count > 0
                    text:               qsTr("Erase All")
                    onClicked:          mainWindow.showMessageDialog(qsTr("Delete All Log Files"),
                                                             qsTr("All log files will be erased permanently. Is this really what you want?"),
                                                             Dialog.Yes | Dialog.No,
                                                             function() { logController.eraseAll() })
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Cancel")
                    enabled:            logController.requestingList || logController.downloadingLogs
                    onClicked:          logController.cancel()
                }
            }
        }
    }
}
