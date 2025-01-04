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
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controllers
import QGroundControl.ScreenTools

Item {
    id:         _root

    property bool loaded: false

    Item {
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
                title:          qsTr("Select log save file")
                onAcceptedForSave: (file) => {
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
                    if (checked && loaded) {
                        listview.positionViewAtEnd();
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
            title:      qsTr("Logging categories")
            buttons:    Dialog.Close

            property int enabledCategoryCount: 0

            function clearAllLogging() {
                var logCategories = QGroundControl.loggingCategories()
                for (var category of logCategories) {
                    QGroundControl.setCategoryLoggingOn(category, false)
                }
                QGroundControl.updateLoggingFilterRules()
                categoryRepeater.model = undefined
                categoryRepeater.model = QGroundControl.loggingCategories()
                enabledCategoryCount = 0
                enabledCategoryRepeater.model = undefined
                enabledCategoryRepeater.model = QGroundControl.loggingCategories()
            }

            function updateLoggingCategory(logCategory, checked, rebuildCategoryList) {
                QGroundControl.setCategoryLoggingOn(logCategory, checked)
                QGroundControl.updateLoggingFilterRules()
                enabledCategoryCount = 0
                enabledCategoryRepeater.model = undefined
                enabledCategoryRepeater.model = QGroundControl.loggingCategories()
                if (rebuildCategoryList) {
                    categoryRepeater.model = undefined
                    categoryRepeater.model = QGroundControl.loggingCategories()
                }
            }

            ColumnLayout {
                RowLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2
                    Layout.alignment: Qt.AlignVCenter
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    QGCLabel {
                        text: qsTr("Search:")
                    }

                    QGCTextField {
                        id: searchText
                        text: ""
                        Layout.fillWidth: true
                        enabled: true
                    }

                    QGCButton {
                        text:       qsTr("Clear")
                        onClicked:  searchText.text = ""
                    }
                }

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    Repeater {
                        id:     enabledCategoryRepeater
                        model:  QGroundControl.loggingCategories()

                        QGCCheckBox {
                            text:       modelData
                            visible:    QGroundControl.categoryLoggingOn(modelData)
                            checked:    QGroundControl.categoryLoggingOn(modelData)
                            onClicked:  updateLoggingCategory(modelData, checked, true /* rebuildCategoryList */)

                            Component.onCompleted: enabledCategoryCount += checked ? 1 : 0
                        }
                    }

                    QGCButton {
                        text:       qsTr("Clear All")
                        visible:    enabledCategoryCount > 0
                        onClicked:  clearAllLogging()
                    }
                }

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    Repeater {
                        id:     categoryRepeater
                        model:  QGroundControl.loggingCategories()

                        QGCCheckBox {
                            text:       modelData
                            visible:    searchText.text ? text.match(`(${searchText.text})`, "i") : true
                            checked:    QGroundControl.categoryLoggingOn(modelData)
                            onClicked:  updateLoggingCategory(modelData, checked, false /* rebuildCategoryList */)
                        }
                    }
                }
            }
        }
    }
}

