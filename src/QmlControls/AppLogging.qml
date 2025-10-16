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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id: root

    property bool listViewLoadCompleted: false

    Item {
        id:             panel
        anchors.fill:   parent

        Rectangle {
            id:              logwindow
            anchors.fill:    parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            color:           qgcPal.window

            Component {
                id: delegateItem
                Rectangle {
                    color:  index % 2 == 0 ? qgcPal.window : qgcPal.windowShade
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 0.5 + field.height)
                    width:  listView.width

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
                id:                     listView
                anchors.top:            parent.top
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.bottom:         followTail.top
                anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth
                clip:                   true
                model:                  debugMessageModel
                delegate:               delegateItem
 
                function scrollToEnd() {
                    if (listViewLoadCompleted) {
                        if (followTail.checked) {
                            listView.positionViewAtEnd();
                        }
                    }
                }

                Component.onCompleted: {
                    listViewLoadCompleted = true
                    listView.scrollToEnd()
                }

                Connections {
                    target:         debugMessageModel
                    function onDataChanged(topLeft, bottomRight, roles) { listView.scrollToEnd() }
                }
            }

            QGCFileDialog {
                id:             writeDialog
                folder:         QGroundControl.settingsManager.appSettings.logSavePath
                nameFilters:    [qsTr("Log files (*.txt)"), qsTr("All Files (*)")]
                title:          qsTr("Select log save file")
                onAcceptedForSave: (file) => {
                    debugMessageModel.writeMessages(file);
                    visible = false;
                }
            }

            Connections {
                target:          debugMessageModel
                function onWriteStarted() { writeButton.enabled = false }
                function onWriteFinished(success) { writeButton.enabled = true }
            }

            QGCButton {
                id:              writeButton
                anchors.bottom:  parent.bottom
                anchors.left:    parent.left
                onClicked:       writeDialog.openForSave()
                text:            qsTr("Save App Log")
            }

            QGCLabel {
                id:                     gstLabel
                anchors.left:           writeButton.right
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: gstCombo.verticalCenter
                text:                   qsTr("GStreamer Debug Level")
                visible:                QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
            }

            FactComboBox {
                id:                 gstCombo
                anchors.left:       gstLabel.right
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                anchors.bottom:     parent.bottom
                fact:               QGroundControl.settingsManager.appSettings.gstDebugLevel
                visible:            QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
                sizeToContents:     true
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
                    if (checked && listViewLoadCompleted) {
                        listView.positionViewAtEnd();
                    }
                }
            }

            QGCButton {
                id:             filterButton
                anchors.bottom: parent.bottom
                anchors.right:  parent.right
                text:           qsTr("Set Logging")
                onClicked:      filtersDialogComponent.createObject(mainWindow).open()
            }
        }
    }

    Component {
        id: filtersDialogComponent

        QGCPopupDialog {
            title:      qsTr("Logging")
            buttons:    Dialog.Close

            ColumnLayout {
                width: maxContentAvailableWidth

                SettingsGroupLayout {
                    heading:            qsTr("Search")
                    Layout.fillWidth:   true

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelHeight / 2

                        QGCTextField {
                            Layout.fillWidth:   true
                            id:                 searchText
                            text:               ""
                            enabled:            true
                        }

                        QGCButton {
                            text:       qsTr("Clear")
                            onClicked:  searchText.text = ""
                        }
                    }
                }

                SettingsGroupLayout {
                    heading:            qsTr("Enabled Categories")
                    Layout.fillWidth:   true

                    Flow {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelHeight / 2

                        Repeater {
                            model: QGroundControl.flatLoggingCategoriesModel()

                            QGCCheckBoxSlider {
                                Layout.fillWidth:       true
                                Layout.maximumHeight:   visible ? implicitHeight : 0
                                text:                   object.fullCategory
                                visible:                object.enabled
                                checked:                object.enabled
                                onClicked:              object.enabled = checked
                            }
                        }

                        QGCButton {
                            text:       qsTr("Disable All")
                            onClicked:  QGroundControl.disableAllLoggingCategories()
                        }
                    }
                }

                // Shown when not filtered
                Flow {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelHeight / 2
                    visible:            searchText.text === ""

                    Repeater {
                        model: QGroundControl.treeLoggingCategoriesModel()

                        ColumnLayout {
                            spacing: ScreenTools.defaultFontPixelHeight / 2

                            RowLayout {
                                spacing:                ScreenTools.defaultFontPixelWidth

                                QGCLabel {
                                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth
                                    text:                   object.expanded ? qsTr("-") : qsTr("+")
                                    horizontalAlignment:    Text.AlignLeft
                                    visible:                object.children

                                    QGCMouseArea {
                                        anchors.fill:   parent
                                        onClicked:      object.expanded = !object.expanded
                                    }
                                }

                                QGCCheckBoxSlider {
                                    Layout.fillWidth:   true
                                    text:               object.shortCategory
                                    checked:            object.enabled
                                    onClicked:          object.enabled = checked
                                }
                            }

                            Repeater {
                                model: object.expanded ? object.children : undefined

                                QGCCheckBoxSlider {
                                    Layout.fillWidth:   true
                                    text:               "   " + object.shortCategory
                                    checked:            object.enabled
                                    onClicked:          object.enabled = checked
                                }
                            }
                        }
                    }
                }

                // Shown when filtered
                Flow {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelHeight / 2
                    visible:            searchText.text !== ""

                    Repeater {
                        model: QGroundControl.flatLoggingCategoriesModel()

                        QGCCheckBoxSlider {
                            Layout.fillWidth:       true
                            Layout.maximumHeight:   visible ? implicitHeight : 0
                            text:                   object.fullCategory
                            visible:                text.match(`(${searchText.text})`, "i")
                            checked:                object.enabled
                            onClicked:              object.enabled = checked
                        }
                    }
                }
            }
        }
    }
}

