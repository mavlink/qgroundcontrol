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

    property real _margin:          ScreenTools.defaultFontPixelWidth
    property real _butttonWidth:    ScreenTools.defaultFontPixelWidth * 10

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    Component {
        id: pageComponent

        RowLayout {
            width:  availableWidth
            height: availableHeight

            function columnWidthProvider(column) {
                switch (column) {
                case 0:
                    return ScreenTools.defaultFontPixelWidth * 2
                case 1:
                    return ScreenTools.defaultFontPixelWidth * 2
                case 2:
                    return ScreenTools.defaultFontPixelWidth * 15
                case 3:
                    return ScreenTools.defaultFontPixelWidth * 10
                case 4:
                    return  ScreenTools.defaultFontPixelWidth * 15
                default:
                    return 0
                }
            }

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
                        Layout.preferredWidth:  columnWidthProvider(1)
                        text:                   qsTr("Id")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel {
                            Layout.preferredWidth:  columnWidthProvider(1)
                            text:                   object.id 
                        }
                    }

                    QGCLabel {
                        Layout.preferredWidth:  columnWidthProvider(2)
                        text:                   qsTr("Date")
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
                        Layout.preferredWidth:  columnWidthProvider(3)
                        text:                   qsTr("Size")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel { text: object.sizeStr }
                    }

                    QGCLabel { 
                        Layout.preferredWidth:  columnWidthProvider(4)
                        text:                   qsTr("Status")
                    }

                    Repeater {
                        model: logController.model

                        QGCLabel { text: object.status }
                    }
                }
            }

            Column {
                spacing:            _margin
                Layout.alignment:   Qt.AlignTop | Qt.AlignLeft
                QGCButton {
                    enabled:    !logController.requestingList && !logController.downloadingLogs
                    text:       qsTr("Refresh")
                    width:      _butttonWidth
                    onClicked: {
                        if (!QGroundControl.multiVehicleManager.activeVehicle || QGroundControl.multiVehicleManager.activeVehicle.isOfflineEditingVehicle) {
                            mainWindow.showMessageDialog(qsTr("Log Refresh"), qsTr("You must be connected to a vehicle in order to download logs."))
                        } else {
                            logController.refresh()
                        }
                    }
                }
                QGCButton {
                    enabled:    !logController.requestingList && !logController.downloadingLogs
                    text:       qsTr("Download")
                    width:      _butttonWidth

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
                    enabled:    !logController.requestingList && !logController.downloadingLogs && logController.model.count > 0
                    text:       qsTr("Erase All")
                    width:      _butttonWidth
                    onClicked:  mainWindow.showMessageDialog(qsTr("Delete All Log Files"),
                                                             qsTr("All log files will be erased permanently. Is this really what you want?"),
                                                             Dialog.Yes | Dialog.No,
                                                             function() { logController.eraseAll() })
                }

                QGCButton {
                    text:       qsTr("Cancel")
                    width:      _butttonWidth
                    enabled:    logController.requestingList || logController.downloadingLogs
                    onClicked:  logController.cancel()
                }
            }
        }
    }
}
