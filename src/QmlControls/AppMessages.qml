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

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    property bool loaded: false

    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }

    Component {
        id: filtersDialogComponent

        QGCViewDialog {
            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  categoryColumn.height
                clip:           true

                Column {
                    id:         categoryColumn
                    spacing:    ScreenTools.defaultFontPixelHeight / 2

                    Repeater {
                        model:      QGroundControl.loggingCategories()

                        QGCCheckBox {
                            text:       modelData
                            checked:    QGroundControl.categoryLoggingOn(modelData)
                            onClicked:  {
                                QGroundControl.setCategoryLoggingOn(modelData, checked)
                                QGroundControl.updateLoggingFilterRules()
                            }
                        }
                    }
                }
            }
        } // QGCViewDialog
    } // Component - filtersDialogComponent

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Rectangle {
            id:              logwindow
            anchors.fill:    parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            color:           qgcPal.window

            Connections {
                target: debugMessageModel

                onDataChanged: {
                    // Keep the view in sync if the button is checked
                    if (loaded) {
                        if (followTail.checked) {
                            listview.positionViewAtEnd();
                        }
                    }
                }
            }

            Component {
                id: delegateItem
                Rectangle {
                    color:  index % 2 == 0 ? qgcPal.window : qgcPal.windowShade
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 0.5 + field.height)
                    width:  listview.width

                    QGCLabel {
                        id:         field
                        text:       display
                        width:      parent.width
                        wrapMode:   Text.Wrap
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            ListView {
                Component.onCompleted: {
                    loaded = true
                }
                anchors.top:     parent.top
                anchors.left:    parent.left
                anchors.right:   parent.right
                anchors.bottom:  followTail.top
                anchors.bottomMargin: ScreenTools.defaultFontPixelWidth
                clip:            true
                id:              listview
                model:           debugMessageModel
                delegate:        delegateItem
            }

            FileDialog {
                id:             writeDialog
                folder:         shortcuts.home
                nameFilters:    [qsTr("Log files (*.txt)"), qsTr("All Files (*)")]
                selectExisting: false
                title:          qsTr("Select log save file")
                onAccepted: {
                    debugMessageModel.writeMessages(fileUrl);
                    visible = false;
                }
                onRejected:     visible = false
            }

            Connections {
                target:          debugMessageModel
                onWriteStarted:  writeButton.enabled = false;
                onWriteFinished: writeButton.enabled = true;
            }

            QGCButton {
                id:              writeButton
                anchors.bottom:  parent.bottom
                anchors.left:    parent.left
                onClicked:       writeDialog.visible = true
                text:            qsTr("Save App Log")
            }

            BusyIndicator {
                id:              writeBusy
                anchors.bottom:  writeButton.bottom
                anchors.left:    writeButton.right
                height:          writeButton.height
                visible:        !writeButton.enabled
            }

            QGCButton {
                id:                     followTail
                anchors.right:          filterButton.left
                anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                anchors.bottom:         parent.bottom
                text:                   qsTr("Show Latest")
                checkable:              true
                checked:                true

                onCheckedChanged: {
                    if (checked && loaded) {
                        listview.positionViewAtEnd();
                    }
                }
            }

            QGCButton {
                id:             filterButton
                anchors.bottom: parent.bottom
                anchors.right:  parent.right
                text:           qsTr("Set logging")
                onClicked:      showDialog(filtersDialogComponent, qsTr("Turn on logging categories"), qgcView.showDialogDefaultWidth, StandardButton.Close)
            }
        }
    } // QGCViewPanel
} // QGCView

