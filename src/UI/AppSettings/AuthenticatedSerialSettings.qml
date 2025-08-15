/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

ColumnLayout {
    spacing: _rowSpacing

    function saveSettings() {
        // No Need - auto-saved through bindings
    }

    // Authentication Settings Group
    GroupBox {
        title: qsTr("Authentication Settings")
        Layout.fillWidth: true
        ColumnLayout {
            spacing: _rowSpacing
            GridLayout {
                columns: 2
                rowSpacing: _rowSpacing
                columnSpacing: _colSpacing

                QGCLabel { text: qsTr("Username") }
                QGCTextField {
                    id: usernameField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.username
                    placeholderText: qsTr("Enter username")
                    onTextChanged: subEditConfig.username = text
                }

                QGCLabel { text: qsTr("Password") }
                QGCTextField {
                    id: passwordField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.password
                    placeholderText: qsTr("Enter password")
                    echoMode: TextInput.Password
                    onTextChanged: subEditConfig.password = text
                }

                QGCLabel { text: qsTr("Auth Server") }
                QGCTextField {
                    id: authHostField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.authHost
                    placeholderText: qsTr("127.0.0.1")
                    onTextChanged: subEditConfig.authHost = text
                }

                QGCLabel { text: qsTr("Auth Port") }
                QGCTextField {
                    id: authPortField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.authPort.toString()
                    placeholderText: qsTr("8080")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: subEditConfig.authPort = parseInt(text) || 8080
                }
            }

            QGCLabel { 
                text: qsTr("Custom Auth URL (optional)")
                Layout.columnSpan: 2
                font.italic: true
            }
            
            QGCTextField {
                id: authUrlField
                Layout.preferredWidth: _secondColumnWidth * 2
                Layout.columnSpan: 2
                text: subEditConfig.authUrl || ""
                placeholderText: qsTr("e.g., http://auth.server.com:8080 or 192.168.1.100:9090")
                onTextChanged: subEditConfig.authUrl = text
            }
        }
    }

    // Data Server Settings Group  
    GroupBox {
        title: qsTr("Data Server Settings")
        Layout.fillWidth: true
        ColumnLayout {
            spacing: _rowSpacing
            GridLayout {
                columns: 2
                rowSpacing: _rowSpacing
                columnSpacing: _colSpacing

                QGCLabel { text: qsTr("Data Server") }
                QGCTextField {
                    id: dataHostField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.dataHost
                    placeholderText: qsTr("127.0.0.1")
                    onTextChanged: subEditConfig.dataHost = text
                }

                QGCLabel { text: qsTr("Data Port") }
                QGCTextField {
                    id: dataPortField
                    Layout.preferredWidth: _secondColumnWidth
                    text: subEditConfig.dataPort.toString()
                    placeholderText: qsTr("5760")
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    onTextChanged: subEditConfig.dataPort = parseInt(text) || 5760
                }
            }
            
            QGCLabel {
                text: qsTr("This is where MAVLink data will be transmitted after authentication")
                font.italic: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    QGCCheckBox {
        id: advancedSettings
        text: qsTr("Advanced Connection Settings")
        checked: false
    }

    GroupBox {
        title: qsTr("Advanced Connection Settings")
        Layout.fillWidth: true
        visible: advancedSettings.checked
        ColumnLayout {
            spacing: _rowSpacing
            
            QGCLabel {
                text: qsTr("Connection Timeout: 10 seconds")
                font.italic: true
            }
            
            QGCLabel {
                text: qsTr("Keep-alive: Automatic")
                font.italic: true
            }
            
            QGCLabel {
                text: qsTr("Protocol: TCP with session authentication")
                font.italic: true
            }
        }
    }

    // Status display
    GroupBox {
        title: qsTr("Connection Status")
        Layout.fillWidth: true

        QGCLabel {
            text: subEditConfig ? 
                  qsTr("Auth: %1 | Data: %2:%3 | User: %4").arg(
                      subEditConfig.authUrl || (subEditConfig.authHost + ":" + subEditConfig.authPort))
                      .arg(subEditConfig.dataHost || "127.0.0.1")
                      .arg(subEditConfig.dataPort || 5760)
                      .arg(subEditConfig.username || qsTr("Not set")) :
                  qsTr("No configuration")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
}
