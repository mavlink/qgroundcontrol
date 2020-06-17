/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts      1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

AnalyzePage {
    id:              mavlinkConsolePage
    pageComponent:   pageComponent
    pageName:        qsTr("Mavlink Console")
    pageDescription: qsTr("Mavlink Console provides a connection to the vehicle's system shell.")

    property bool isLoaded: false

    MavlinkConsoleController {
        id: conController
    }

    Component {
        id: pageComponent

        ColumnLayout {
            id:     consoleColumn
            height: availableHeight
            width:  availableWidth

            Connections {
                target: conController

                onDataChanged: {
                    // Keep the view in sync if the button is checked
                    if (isLoaded) {
                        if (followTail.checked) {
                            listview.positionViewAtEnd();
                        }
                    }
                }
            }

            Component {
                id: delegateItem
                Rectangle {
                    color:  qgcPal.windowShade
                    height: Math.round(ScreenTools.defaultFontPixelHeight * 0.1 + field.height)
                    width:  listview.width

                    QGCLabel {
                        id:          field
                        text:        display
                        width:       parent.width
                        wrapMode:    Text.NoWrap
                        font.family: ScreenTools.fixedFontFamily
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            QGCListView {
                Component.onCompleted: {
                    isLoaded = true
                }
                Layout.fillHeight: true
                Layout.fillWidth:  true
                clip:              true
                id:                listview
                model:             conController
                delegate:          delegateItem

                // Unsync the view if the user interacts
                onMovementStarted: {
                    followTail.checked = false
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                QGCTextField {
                    id:               command
                    Layout.fillWidth: true
                    placeholderText:  "Enter Commands here..."
                    inputMethodHints: Qt.ImhNoAutoUppercase

                    function sendCommand() {
                        conController.sendCommand(text)
                        text = ""
                    }
                    onAccepted: sendCommand()

                    Keys.onPressed: {
                        if (event.key === Qt.Key_Up) {
                            text = conController.historyUp(text);
                            event.accepted = true;
                        } else if (event.key === Qt.Key_Down) {
                            text = conController.historyDown(text);
                            event.accepted = true;
                        }
                    }
                }

                QGCButton {
                    id:        sendButton
                    text:      qsTr("Send")
                    visible:   ScreenTools.isMobile

                    onClicked: command.sendCommand()
                }

                QGCButton {
                    id:        followTail
                    text:      qsTr("Show Latest")
                    checkable: true
                    checked:   true

                    onCheckedChanged: {
                        if (checked && isLoaded) {
                            listview.positionViewAtEnd();
                        }
                    }
                }
            }
        }
    } // Component
} // AnalyzePage
