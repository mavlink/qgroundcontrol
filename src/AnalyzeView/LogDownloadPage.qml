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

            Connections {
                target: logController
                onSelectionChanged: {
                    tableView.selection.clear()
                    for(var i = 0; i < logController.model.count; i++) {
                        var o = logController.model.get(i)
                        if (o && o.selected) {
                            tableView.selection.select(i, i)
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                Layout.fillHeight:  true
                spacing:            0

                HorizontalHeaderView {
                    Layout.fillWidth:   true
                    model:              [ qsTr("Id"), qsTr("Date"), qsTr("Size"), qsTr("Status") ]
                    syncView:           tableView
                    clip:               true
                }

                TableView {
                    id:                     tableView
                    Layout.fillWidth:       true
                    model:                  logController.model
                    selectionBehavior:      TableView.SelectRows
                    selectionMode:          TableView.ExtendedSelection
                    columnWidthProvider:    columnWidth
                    selectionModel:         ItemSelectionModel {}

                    property int modelCount: model.count

                    onModelCountChanged: console.log("modelCount", modelCount)

                    onColumnsChanged: console.log("columns", columns)

                    function columnWidth(column) {
                        switch (column) {
                        case 0:
                            return ScreenTools.defaultFontPixelWidth * 6
                        case 1:
                            return ScreenTools.defaultFontPixelWidth * 34
                        case 2:
                            return ScreenTools.defaultFontPixelWidth * 18
                        case 3:
                            return  ScreenTools.defaultFontPixelWidth * 22
                        default:
                            return 0
                        }
                    }

                    delegate: DelegateChooser {
                        DelegateChoice { 
                            column: 0
                            delegate : Text  {
                                color: qgcPal.text
                                //horizontalAlignment: Text.AlignHCenter
                                Component.onCompleted: console.log("index", index, text)
                                text: {
                                    var o = logController.model.get(index)
                                    return o ? o.id : ""
                                }
                            }
                        }

                        DelegateChoice { 
                            column: 1
                            delegate: Text  {
                                color: qgcPal.text
                                text: {
                                    var o = logController.model.get(index)
                                    if (o) {
                                        //-- Have we received this entry already?
                                        if(logController.model.get(index).received) {
                                            var d = logController.model.get(index).time
                                            if(d.getUTCFullYear() < 2010)
                                                return qsTr("Date Unknown")
                                            else
                                                return d.toLocaleString(undefined, "short")
                                        }
                                    }
                                    return ""
                                }
                            }
                        }

                        DelegateChoice { 
                            column: 2
                            delegate : Text  {
                                color: qgcPal.text
                                //horizontalAlignment: Text.AlignRight
                                Component.onCompleted: console.log("size", index, logController.model.get(index).sizeStr)
                                text: {
                                    var o = logController.model.get(index)
                                    return o ? o.sizeStr : ""
                                }
                            }
                        }

                        DelegateChoice { 
                            column: 3
                            delegate : Text  {
                                color: qgcPal.text
                                //horizontalAlignment: Text.AlignHCenter
                                text: {
                                    var o = logController.model.get(index)
                                    return o ? o.status : ""
                                }
                            }
                        }
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
                    enabled:    !logController.requestingList && !logController.downloadingLogs && tableView.selectionModel.selectedIndexes.count > 0
                    text:       qsTr("Download")
                    width:      _butttonWidth
                    onClicked: {
                        //-- Clear selection
                        for(var i = 0; i < logController.model.count; i++) {
                            var o = logController.model.get(i)
                            if (o) o.selected = false
                        }
                        //-- Flag selected log files
                        tableView.selectionModel.selectedIndexes.forEach(function(rowIndex){
                            var o = logController.model.get(rowIndex)
                            if (o) o.selected = true
                        })
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
