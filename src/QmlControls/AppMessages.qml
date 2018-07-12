/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    property bool loaded: false

    property var _qgcView: qgcView

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

                    QGCButton {
                        text: qsTr("Clear All")
                        onClicked: {
                            var logCats = QGroundControl.loggingCategories()
                            for (var i=0; i<logCats.length; i++) {
                                QGroundControl.setCategoryLoggingOn(logCats[i], false)
                            }
                            QGroundControl.updateLoggingFilterRules()
                            categoryRepeater.model = undefined
                            categoryRepeater.model = QGroundControl.loggingCategories()
                        }
                    }

                    Repeater {
                        id:     categoryRepeater
                        model:  QGroundControl.loggingCategories()

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

            QGCListView {
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

            QGCFileDialog {
                id:             writeDialog
                folder:         QGroundControl.settingsManager.appSettings.logSavePath
                nameFilters:    [qsTr("Log files (*.txt)"), qsTr("All Files (*)")]
                selectExisting: false
                title:          qsTr("Select log save file")
                qgcView:        _qgcView
                onAcceptedForSave: {
                    debugMessageModel.writeMessages(file);
                    visible = false;
                }
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
                onClicked:       writeDialog.openForSave()
                text:            qsTr("Save App Log")
            }

            QGCLabel {
                id:                  gstLabel
                anchors.baseline:    gstCombo.baseline
                anchors.right:       gstCombo.left
                anchors.rightMargin: ScreenTools.defaultFontPixelWidth
                text:                "gstreamer debug level:"
            }

            FactComboBox {
                id:                  gstCombo
                anchors.right:       followTail.left
                anchors.rightMargin: ScreenTools.defaultFontPixelWidth*20
                anchors.bottom:      parent.bottom
                width:               ScreenTools.defaultFontPixelWidth*20
                model:               ["disabled", "1", "2", "3", "4", "5", "6", "7", "8"]
                fact:                QGroundControl.settingsManager.appSettings.gstDebug
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

