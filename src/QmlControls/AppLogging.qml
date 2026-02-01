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

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Item {
        id:             panel
        anchors.fill:   parent

        Rectangle {
            id:              logwindow
            anchors.fill:    parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            color:           qgcPal.window

            // Error banner at top
            Rectangle {
                id:             errorBanner
                anchors.top:    parent.top
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         visible ? errorRow.height + ScreenTools.defaultFontPixelHeight : 0
                color:          qgcPal.warningText
                visible:        qgcLogging.hasError

                RowLayout {
                    id:                 errorRow
                    anchors.centerIn:   parent
                    spacing:            ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text:   qsTr("Logging error occurred")
                        color:  "white"
                    }

                    QGCButton {
                        text:       qsTr("Clear")
                        onClicked:  qgcLogging.clearError()
                    }
                }
            }

            Component {
                id: delegateItem
                Rectangle {
                    color:  index % 2 == 0 ? qgcPal.window : qgcPal.windowShade
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 0.5 + field.height)
                    width:  listView.width

                    QGCLabel {
                        id:         field
                        text:       formatted
                        width:      parent.width
                        wrapMode:   Text.Wrap
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            QGCListView {
                id:                     listView
                anchors.top:            errorBanner.bottom
                anchors.topMargin:      errorBanner.visible ? ScreenTools.defaultFontPixelWidth : 0
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.bottom:         buttonRow.top
                anchors.bottomMargin:   ScreenTools.defaultFontPixelWidth
                clip:                   true
                model:                  qgcLogging.model
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
                    target:         qgcLogging.model
                    function onCountChanged() { listView.scrollToEnd() }
                }
            }

            QGCFileDialog {
                id:             writeDialog
                folder:         QGroundControl.settingsManager.appSettings.logSavePath
                nameFilters:    [qsTr("Log files (*.txt)"), qsTr("All Files (*)")]
                title:          qsTr("Select log save file")
                onAcceptedForSave: (file) => {
                    qgcLogging.exportToFile(file);
                    visible = false;
                }
            }

            Connections {
                target:          qgcLogging
                function onExportStarted() { writeButton.enabled = false }
                function onExportFinished(success) { writeButton.enabled = true }
            }

            RowLayout {
                id:             buttonRow
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        ScreenTools.defaultFontPixelWidth

                QGCButton {
                    id:         writeButton
                    text:       qsTr("Save App Log")
                    onClicked:  writeDialog.openForSave()
                }

                QGCLabel {
                    id:         gstLabel
                    text:       qsTr("GStreamer Debug Level")
                    visible:    QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
                }

                FactComboBox {
                    id:             gstCombo
                    fact:           QGroundControl.settingsManager.appSettings.gstDebugLevel
                    visible:        QGroundControl.settingsManager.appSettings.gstDebugLevel.visible
                    sizeToContents: true
                }

                Item { Layout.fillWidth: true }

                // Status indicators
                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth / 2

                    Rectangle {
                        width:          ScreenTools.defaultFontPixelHeight * 0.75
                        height:         width
                        radius:         width / 2
                        color:          qgcLogging.diskLoggingEnabled ? "green" : qgcPal.windowShade
                        border.color:   qgcPal.text
                        border.width:   1

                        QGCMouseArea {
                            anchors.fill:   parent
                            hoverEnabled:   true
                            cursorShape:    Qt.PointingHandCursor
                            onClicked:      qgcLogging.diskLoggingEnabled = !qgcLogging.diskLoggingEnabled

                            ToolTip.visible: containsMouse
                            ToolTip.text:    qsTr("Disk Logging: %1").arg(qgcLogging.diskLoggingEnabled ? qsTr("On") : qsTr("Off"))
                        }
                    }

                    Rectangle {
                        width:          ScreenTools.defaultFontPixelHeight * 0.75
                        height:         width
                        radius:         width / 2
                        color:          qgcLogging.remoteLoggingEnabled ? "blue" : qgcPal.windowShade
                        border.color:   qgcPal.text
                        border.width:   1

                        QGCMouseArea {
                            anchors.fill:   parent
                            hoverEnabled:   true
                            cursorShape:    Qt.PointingHandCursor
                            onClicked:      settingsDialogComponent.createObject(mainWindow).open()

                            ToolTip.visible: containsMouse
                            ToolTip.text:    qsTr("Remote Logging: %1").arg(qgcLogging.remoteLoggingEnabled ? qsTr("On") : qsTr("Off"))
                        }
                    }
                }

                QGCButton {
                    id:         followTail
                    text:       qsTr("Show Latest")
                    checkable:  true
                    checked:    true

                    onCheckedChanged: {
                        if (checked && listViewLoadCompleted) {
                            listView.positionViewAtEnd();
                        }
                    }
                }

                QGCButton {
                    text:       qsTr("Categories")
                    onClicked:  categoriesDialogComponent.createObject(mainWindow).open()
                }

                QGCButton {
                    text:       qsTr("Settings")
                    onClicked:  settingsDialogComponent.createObject(mainWindow).open()
                }
            }
        }
    }

    Component {
        id: settingsDialogComponent
        LogSettingsDialog { }
    }

    Component {
        id: categoriesDialogComponent
        LogCategoriesDialog { }
    }
}
