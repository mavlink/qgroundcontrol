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
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Controllers

AnalyzePage {
    id: root
    pageComponent: pageComponent
    pageDescription: qsTr("Provides a connection to the vehicle's system shell.")
    allowPopout: true

    property bool isLoaded: false

    // Key input on mobile is handled differently, so use a separate command input text field.
    // E.g. for android see https://bugreports.qt.io/browse/QTBUG-40803
    readonly property bool _separateCommandInput: ScreenTools.isMobile

    MAVLinkConsoleController { id: conController }

    Component {
        id: pageComponent

        ColumnLayout {
            height: availableHeight
            width: availableWidth
            property int _consoleOutputLen: 0

            function scrollToBottom() {
                if (flickable.contentHeight > flickable.height) {
                    flickable.contentY = flickable.contentHeight - flickable.height
                }
            }

            function getCommand() { return textConsole.getText(_consoleOutputLen, textConsole.length) }

            function getCommandAndClear() {
                const command = getCommand()
                textConsole.remove(_consoleOutputLen, textConsole.length)
                return command
            }

            function pasteFromClipboard() {
                // we need to handle a few cases here:
                // in the general form we have: <command_pre><cursor><command_post>
                // and the clipboard may contain newlines
                const cursor = textConsole.cursorPosition - _consoleOutputLen
                var command = getCommandAndClear()
                var command_pre = ""
                var command_post = command
                if (cursor > 0) {
                    command_pre = command.substr(0, cursor)
                    command_post = command.substr(cursor)
                }
                var command_leftover = conController.handleClipboard(command_pre) + command_post
                textConsole.insert(textConsole.length, command_leftover)
                textConsole.cursorPosition = textConsole.length - command_post.length
            }

            Connections {
                target: conController

                onDataChanged: {
                    if (isLoaded) {
                        // rate-limit updates to reduce CPU load
                        updateTimer.start();
                    }
                }
            }

            Timer {
                id: updateTimer
                interval: 30
                running: false
                repeat: false
                onTriggered: {
                    // only update if scroll bar is at the bottom
                    if (flickable.atYEnd) {
                        // backup & restore cursor & command
                        const command = getCommand()
                        const cursor = textConsole.cursorPosition - _consoleOutputLen
                        textConsole.text = conController.text
                        _consoleOutputLen = textConsole.length
                        textConsole.insert(textConsole.length, command)
                        textConsole.cursorPosition = textConsole.length
                        scrollToBottom()
                        if (cursor >= 0) {
                            // We could restore the selection here too...
                            textConsole.cursorPosition = _consoleOutputLen + cursor
                        }
                    } else {
                        updateTimer.start();
                    }
                }
            }

            QGCFlickable {
                id: flickable
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: textConsole.width
                contentHeight: textConsole.height

                TextArea.flickable: TextArea {
                    id: textConsole
                    width: availableWidth
                    wrapMode: Text.WordWrap
                    readOnly: _separateCommandInput
                    textFormat: TextEdit.RichText
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhMultiLine
                    text: "> "
                    focus: true
                    color: qgcPal.text
                    selectedTextColor: qgcPal.windowShade
                    selectionColor: qgcPal.text
                    font.pointSize: ScreenTools.defaultFontPointSize
                    font.family: ScreenTools.fixedFontFamily

                    Component.onCompleted: {
                        root.isLoaded = true
                        _consoleOutputLen = textConsole.length
                        textConsole.cursorPosition = _consoleOutputLen
                        if (!_separateCommandInput) {
                            textConsole.forceActiveFocus()
                        }
                    }

                    background: Rectangle { color: qgcPal.windowShade }

                    Keys.onPressed: (event) => {
                        // ignore tabs
                        if (event.key == Qt.Key_Tab) {
                            event.accepted = true
                        }

                        // ignore for now
                        if (event.matches(StandardKey.Cut)) {
                            event.accepted = true
                        }

                        if (!event.matches(StandardKey.Copy) &&
                            event.key != Qt.Key_Escape &&
                            event.key != Qt.Key_Insert &&
                            event.key != Qt.Key_Pause &&
                            event.key != Qt.Key_Print &&
                            event.key != Qt.Key_SysReq &&
                            event.key != Qt.Key_Clear &&
                            event.key != Qt.Key_Home &&
                            event.key != Qt.Key_End &&
                            event.key != Qt.Key_Left &&
                            event.key != Qt.Key_Up &&
                            event.key != Qt.Key_Right &&
                            event.key != Qt.Key_Down &&
                            event.key != Qt.Key_PageUp &&
                            event.key != Qt.Key_PageDown &&
                            event.key != Qt.Key_Shift &&
                            event.key != Qt.Key_Control &&
                            event.key != Qt.Key_Meta &&
                            event.key != Qt.Key_Alt &&
                            event.key != Qt.Key_AltGr &&
                            event.key != Qt.Key_CapsLock &&
                            event.key != Qt.Key_NumLock &&
                            event.key != Qt.Key_ScrollLock &&
                            event.key != Qt.Key_Super_L &&
                            event.key != Qt.Key_Super_R &&
                            event.key != Qt.Key_Menu &&
                            event.key != Qt.Key_Hyper_L &&
                            event.key != Qt.Key_Hyper_R &&
                            event.key != Qt.Key_Direction_L &&
                            event.key != Qt.Key_Direction_R) {
                            // Note: dead keys do not generate keyPressed event on linux, see
                            // https://bugreports.qt.io/browse/QTBUG-79216

                            scrollToBottom()

                            // ensure cursor position is at an editable region
                            if (textConsole.selectionStart < _consoleOutputLen) {
                                textConsole.select(_consoleOutputLen, textConsole.selectionEnd)
                            }

                            if (textConsole.cursorPosition < _consoleOutputLen) {
                                textConsole.cursorPosition = textConsole.length
                            }
                        }

                        switch (event.key) {
                        case Qt.Key_Left:
                            // don't move beyond current command
                            if (textConsole.cursorPosition == _consoleOutputLen) {
                                event.accepted = true
                            }
                            break;
                        case Qt.Key_Backspace:
                            if (textConsole.cursorPosition <= _consoleOutputLen) {
                                event.accepted = true
                            }
                            break;
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            conController.sendCommand(getCommandAndClear())
                            event.accepted = true
                            break;
                        default:
                            break;
                        }

                        if (event.matches(StandardKey.Paste)) {
                            pasteFromClipboard()
                            event.accepted = true
                        }

                        // command history
                        if (event.key == Qt.Key_Up) {
                            const command = conController.historyUp(getCommandAndClear())
                            textConsole.insert(textConsole.length, command)
                            textConsole.cursorPosition = textConsole.length
                            event.accepted = true
                        } else if (event.key == Qt.Key_Down) {
                            const command = conController.historyDown(getCommandAndClear())
                            textConsole.insert(textConsole.length, command)
                            textConsole.cursorPosition = textConsole.length
                            event.accepted = true
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: _separateCommandInput

                QGCTextField {
                    id: commandInput
                    Layout.fillWidth: true
                    placeholderText:  qsTr("Enter Commands here...")
                    inputMethodHints: Qt.ImhNoAutoUppercase
                    onAccepted: sendCommand()

                    function sendCommand() {
                        conController.sendCommand(text)
                        text = ""
                        scrollToBottom()
                    }

                }

                QGCButton {
                    text: qsTr("Send")
                    onClicked: commandInput.sendCommand()
                }
            }
        }
    } // Component
} // AnalyzePage
