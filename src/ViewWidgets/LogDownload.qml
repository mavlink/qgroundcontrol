/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    viewPanel:  panel

    property real _margins:         ScreenTools.defaultFontPixelHeight
    property real _butttonWidth:    ScreenTools.defaultFontPixelWidth * 10

    LogDownloadController {
        id:         controller
        factPanel:  panel
        onSelectionChanged: {
            tableView.selection.clear()
            for(var i = 0; i < controller.model.count; i++) {
                var o = controller.model.get(i)
                if (o && o.selected) {
                    tableView.selection.select(i, i)
                }
            }
        }
    }

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        TableView {
            id: tableView
            anchors.margins:    _margins
            anchors.left:       parent.left
            anchors.right:      refreshButton.left
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            model:              controller.model
            selectionMode:      SelectionMode.MultiSelection

            TableViewColumn {
                title: qsTr("Id")
                width: ScreenTools.defaultFontPixelWidth * 6
                horizontalAlignment: Text.AlignHCenter
                delegate : Text  {
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var o = controller.model.get(styleData.row)
                        return o ? o.id : ""
                    }
                }
            }

            TableViewColumn {
                title: qsTr("Date")
                width: ScreenTools.defaultFontPixelWidth * 34
                horizontalAlignment: Text.AlignHCenter
                delegate : Text  {
                    text: {
                        var o = controller.model.get(styleData.row)
                        if (o) {
                            //-- Have we received this entry already?
                            if(controller.model.get(styleData.row).received) {
                                var d = controller.model.get(styleData.row).time
                                if(d.getUTCFullYear() < 2010)
                                    return qsTr("Date Unknown")
                                else
                                    return d.toLocaleString()
                            }
                        }
                        return ""
                    }
                }
            }

            TableViewColumn {
                title: qsTr("Size")
                width: ScreenTools.defaultFontPixelWidth * 18
                horizontalAlignment: Text.AlignHCenter
                delegate : Text  {
                    horizontalAlignment: Text.AlignRight
                    text: {
                        var o = controller.model.get(styleData.row)
                        return o ? o.sizeStr : ""
                    }
                }
            }

            TableViewColumn {
                title: qsTr("Status")
                width: ScreenTools.defaultFontPixelWidth * 22
                horizontalAlignment: Text.AlignHCenter
                delegate : Text  {
                    horizontalAlignment: Text.AlignHCenter
                    text: {
                        var o = controller.model.get(styleData.row)
                        return o ? o.status : ""
                    }
                }
            }

        }

        QGCButton {
            id:                 refreshButton
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.right:      parent.right
            enabled:            !controller.requestingList && !controller.downloadingLogs
            text:               qsTr("Refresh")
            width:              _butttonWidth
            onClicked: {
                controller.refresh()
            }
        }

        QGCButton {
            id:                 downloadButton
            anchors.margins:    _margins
            anchors.top:        refreshButton.bottom
            anchors.right:      parent.right
            enabled:            !controller.requestingList && !controller.downloadingLogs && tableView.selection.count > 0
            text:               qsTr("Download")
            width:              _butttonWidth
            onClicked: {
                //-- Clear selection
                for(var i = 0; i < controller.model.count; i++) {
                    var o = controller.model.get(i)
                    if (o) o.selected = false
                }
                //-- Flag selected log files
                tableView.selection.forEach(function(rowIndex){
                    var o = controller.model.get(rowIndex)
                    if (o) o.selected = true
                })
                //-- Download them
                controller.download()
            }
        }

        QGCButton {
            id:                 eraseAllButton
            anchors.margins:    _margins
            anchors.top:        downloadButton.bottom
            anchors.right:      parent.right
            enabled:            !controller.requestingList && !controller.downloadingLogs && controller.model.count > 0
            text:               qsTr("Erase All")
            width:              _butttonWidth
            onClicked: {
                eraseAllDialog.visible = true
            }
            MessageDialog {
                id:         eraseAllDialog
                visible:    false
                icon:       StandardIcon.Warning
                standardButtons: StandardButton.Yes | StandardButton.No
                title:      qsTr("Delete All Log Files")
                text:       qsTr("All log files will be erased permanently. Is this really what you want?")
                onYes: {
                    controller.eraseAll()
                    eraseAllDialog.visible = false
                }
                onNo: {
                    eraseAllDialog.visible = false
                }
            }
        }

        QGCButton {
            id:                 cancelButton
            anchors.margins:    _margins
            anchors.top:        eraseAllButton.bottom
            anchors.right:      parent.right
            text:               qsTr("Cancel")
            width:              _butttonWidth
            enabled:            controller.requestingList || controller.downloadingLogs
            onClicked: {
                controller.cancel()
            }
        }
    }
}
