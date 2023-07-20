/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         2.12
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactControls  1.0
import QGroundControl.FactSystem    1.0


Rectangle {
    id:                 _linkRoot
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var _currentSelection:     null
    property int _firstColumnWidth:     ScreenTools.defaultFontPixelWidth * 12
    property int _secondColumnWidth:    ScreenTools.defaultFontPixelWidth * 30
    property int _rowSpacing:           ScreenTools.defaultFontPixelHeight / 2
    property int _colSpacing:           ScreenTools.defaultFontPixelWidth / 2
    property int _isConnectionServer: {
        if (!QGroundControl.linkManager.isConnectServer) {
            connectingDialog.visible = true
            return false
        }
        return true
    }
    property int _isAuthServer: {
        if (QGroundControl.linkManager.isAuthServer) {
            closeAirLinkRegistration()
            return true
        } else {
            loginDialog.visible = true
            return false
        }
    }

    MessageDialog {
        id:         loginDialog
        visible:    false
        icon:       StandardIcon.Warning
        standardButtons: StandardButton.Yes
        title:      qsTr("AirLink Authentification")
        text:       qsTr("Wrong login or password. Please check it and try again!")

        onYes: loginDialog.visible = false
    }

    MessageDialog {
        id:         connectingDialog
        visible:    false
        icon:       StandardIcon.Warning
        standardButtons: StandardButton.Yes
        title:      qsTr("AirLink Authentification")
        text:       qsTr("No network connection. Please check it and try again!")

        onYes: loginDialog.visible = false
    }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    property bool highlight: pressed | checked | hovered

    function openCommSettings(originalLinkConfig) {
        settingsLoader.originalLinkConfig = originalLinkConfig
        if (originalLinkConfig) {
            // Editing existing link config
            settingsLoader.editingConfig = QGroundControl.linkManager.startConfigurationEditing(originalLinkConfig)
        } else {
            // Create new link configuration
            settingsLoader.editingConfig = QGroundControl.linkManager.createConfiguration(ScreenTools.isSerialAvailable ? LinkConfiguration.TypeSerial : LinkConfiguration.TypeUdp, "")
        }
        settingsLoader.sourceComponent = commSettings
    }

    function openAirLinkRegistration() {
        settingsLoader.sourceComponent = airLinkRegistration
    }

    function closeAirLinkRegistration() {
        settingsLoader.sourceComponent = null
        QGroundControl.linkManager.createConfigurationAirLink()
    }

    Component.onDestruction: {
        if (settingsLoader.sourceComponent) {
            settingsLoader.sourceComponent = null
            QGroundControl.linkManager.cancelConfigurationEditing(settingsLoader.editingConfig)
        }
    }

    QGCFlickable {
        clip:               true
        anchors.top:        parent.top
        width:              parent.width
        height:             parent.height - buttonRow.height
        contentHeight:      settingsColumn.height
        contentWidth:       _linkRoot.width
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:                 settingsColumn
            width:              _linkRoot.width
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            Repeater {
                model: QGroundControl.linkManager.linkConfigurations
                delegate: QGCButton {
                    anchors.horizontalCenter:   settingsColumn.horizontalCenter
                    width:                      _linkRoot.width * 0.5
                    autoExclusive:              true
                    visible:                    !object.dynamic
                    onClicked: {
                        checked = true
                        _currentSelection = object
                        console.log("clicked", object, object.link)
                        buttonRow.update()
                    }
                    Text {
                        anchors.centerIn:   parent
                        text:               object.online ? object.name + " (online)" : object.name                       
                        color:              object.online ? qgcPal.colorGreen : qgcPal.buttonText
                    }
                }
            }
        }
    }

    Row {
        id:                 buttonRow
        spacing:            ScreenTools.defaultFontPixelWidth
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.horizontalCenter: parent.horizontalCenter
        QGCButton {
            text:       qsTr("Login AirLink")
            enabled:    true
            onClicked:  _linkRoot.openAirLinkRegistration()
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       qsTr("Delete")
            enabled:    _currentSelection && !_currentSelection.dynamic
            onClicked:  deleteDialog.visible = true

            MessageDialog {
                id:         deleteDialog
                visible:    false
                icon:       StandardIcon.Warning
                standardButtons: StandardButton.Yes | StandardButton.No
                title:      qsTr("Remove Link Configuration")
                text:       _currentSelection ? qsTr("Remove %1. Is this really what you want?").arg(_currentSelection.name) : ""

                onYes: {
                    QGroundControl.linkManager.removeConfiguration(_currentSelection)
                    _currentSelection = null
                    deleteDialog.visible = false
                }
                onNo: deleteDialog.visible = false
            }
        }
        QGCButton {
            text:       qsTr("Edit")
            enabled:    _currentSelection && !_currentSelection.link
            onClicked:  _linkRoot.openCommSettings(_currentSelection)
        }
        QGCButton {
            text:       qsTr("Add")
            onClicked:  _linkRoot.openCommSettings(null)
        }
        QGCButton {
            text:       qsTr("Connect")
            enabled:    _currentSelection && !_currentSelection.link
            onClicked:  {
                QGroundControl.linkManager.createConnectedLink(_currentSelection)
                QGroundControl.linkManager.sendLoginMsgToAirLink(_currentSelection.link, _currentSelection.name)
            }
        }
        QGCButton {
            text:       qsTr("Disconnect")
            enabled:    _currentSelection && _currentSelection.link
            onClicked:  {
                _currentSelection.link.disconnect()
                _currentSelection.linkChanged()
            }
        }
        QGCButton {
            text:       qsTr("MockLink Options")
            visible:    _currentSelection && _currentSelection.link && _currentSelection.link.isMockLink
            onClicked:  mockLinkOptionDialog.open()

            MockLinkOptionsDlg {
                id:     mockLinkOptionDialog
                link:   _currentSelection ? _currentSelection.link : undefined
            }
        }
    }

    Loader {
        id:             settingsLoader
        anchors.fill:   parent
        visible:        sourceComponent ? true : false

        property var originalLinkConfig:    null
        property var editingConfig:      null
    }

    //---------------------------------------------
    // Comm Settings
    Component {
        id: commSettings
        Rectangle {
            id:             settingsRect
            color:          qgcPal.window
            anchors.fill:   parent
            property real   _panelWidth:    width * 0.8

            QGCFlickable {
                id:                 settingsFlick
                clip:               true
                anchors.fill:       parent
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                contentHeight:      mainLayout.height
                contentWidth:       mainLayout.width

                ColumnLayout {
                    id:         mainLayout
                    spacing:    _rowSpacing

                    QGCGroupBox {
                        title: originalLinkConfig ? qsTr("Edit Link Configuration Settings") : qsTr("Create New Link Configuration")

                        ColumnLayout {
                            spacing: _rowSpacing

                            GridLayout {
                                columns:        2
                                columnSpacing:  _colSpacing
                                rowSpacing:     _rowSpacing

                                QGCLabel { text: qsTr("Name") }
                                QGCTextField {
                                    id:                     nameField
                                    Layout.preferredWidth:  _secondColumnWidth
                                    Layout.fillWidth:       true
                                    text:                   editingConfig.name
                                    placeholderText:        qsTr("Enter name")
                                }

                                QGCCheckBox {
                                    Layout.columnSpan:  2
                                    text:               qsTr("Automatically Connect on Start")
                                    checked:            editingConfig.autoConnect
                                    onCheckedChanged:   editingConfig.autoConnect = checked
                                }

                                QGCCheckBox {
                                    Layout.columnSpan:  2
                                    text:               qsTr("High Latency")
                                    checked:            editingConfig.highLatency
                                    onCheckedChanged:   editingConfig.highLatency = checked
                                }

                                QGCLabel { text: qsTr("Type") }
                                QGCComboBox {
                                    Layout.preferredWidth:  _secondColumnWidth
                                    Layout.fillWidth:       true
                                    enabled:                originalLinkConfig == null
                                    model:                  QGroundControl.linkManager.linkTypeStrings
                                    currentIndex:           editingConfig.linkType

                                    onActivated: {
                                        if (index !== editingConfig.linkType) {
                                            // Save current name
                                            var name = nameField.text
                                            // Create new link configuration
                                            editingConfig = QGroundControl.linkManager.createConfiguration(index, name)
                                        }
                                    }
                                }
                            }

                            Loader {
                                id:     linksettingsLoader
                                source: subEditConfig.settingsURL

                                property var subEditConfig: editingConfig
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment:   Qt.AlignHCenter
                        spacing:            _colSpacing

                        QGCButton {
                            width:      ScreenTools.defaultFontPixelWidth * 10
                            text:       qsTr("OK")
                            enabled:    nameField.text !== ""

                            onClicked: {
                                // Save editing
                                linksettingsLoader.item.saveSettings()
                                editingConfig.name = nameField.text
                                settingsLoader.sourceComponent = null
                                if (originalLinkConfig) {
                                    QGroundControl.linkManager.endConfigurationEditing(originalLinkConfig, editingConfig)
                                } else {
                                    // If it was edited, it's no longer "dynamic"
                                    editingConfig.dynamic = false
                                    QGroundControl.linkManager.endCreateConfiguration(editingConfig)
                                }
                            }
                        }

                        QGCButton {
                            width:      ScreenTools.defaultFontPixelWidth * 10
                            text:       qsTr("Cancel")
                            onClicked: {
                                settingsLoader.sourceComponent = null
                                QGroundControl.linkManager.cancelConfigurationEditing(settingsLoader.editingConfig)
                            }
                        }
                    }
                }
            }
        }
    }

    //---------------------------------------------
    // AirLink Registration
    Component {
        id: airLinkRegistration
        Rectangle {
            id:             settingsRect
            color:          qgcPal.window
            anchors.fill:   parent
            property real   _panelWidth:    width * 0.8

            QGCFlickable {
                id:                 settingsFlick
                clip:               true
                anchors.fill:       parent
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                contentHeight:      mainLayout.height
                contentWidth:       mainLayout.width

                ColumnLayout {
                    id:         mainLayout
                    spacing:    _rowSpacing

                    QGCGroupBox {
                        title: qsTr("Login / Registration")

                        ColumnLayout {
                            spacing: _rowSpacing

                            GridLayout {
                                columns:        2
                                columnSpacing:  _colSpacing
                                rowSpacing:     _rowSpacing

                                QGCLabel {text: qsTr("User Name:")}
                                FactTextField {
                                    id:             _userText
                                    fact:           _usernameFact
                                    width:          _secondColumnWidth
                                    visible:        _usernameFact.visible
                                    placeholderText:qsTr("Enter Login")
                                    Layout.fillWidth:    true
                                    Layout.preferredWidth:  _secondColumnWidth
                                    property Fact _usernameFact: QGroundControl.settingsManager.appSettings.loginAirLink
                                }
                                QGCLabel { text: qsTr("Password:") }
                                FactTextField {
                                    id:             _passText
                                    fact:           _passwordFact
                                    width:          _secondColumnWidth
                                    visible:        _passwordFact.visible
                                    placeholderText:qsTr("Enter Password")
                                    echoMode:       TextInput.Password
                                    Layout.fillWidth:    true
                                    property Fact _passwordFact: QGroundControl.settingsManager.appSettings.passAirLink
                                }
                                QGCLabel {
                                    text: "Forgot Your AirLink Password?"
                                    font.underline: true
                                    Layout.columnSpan:  2
                                    MouseArea {
                                        anchors.fill:   parent
                                        hoverEnabled:   true
                                        cursorShape:    Qt.PointingHandCursor
                                        onClicked:      Qt.openUrlExternally("https://air-link.space/forgot-pass")
                                    }
                                }
                            }
                        }
                    }


                    RowLayout {
                        Layout.alignment:   Qt.AlignHCenter
                        spacing:            _colSpacing

                        QGCButton {
                            width:      ScreenTools.defaultFontPixelWidth * 10
                            text:       qsTr("Register")
                            onClicked:  Qt.openUrlExternally("https://air-link.space/registration")
                        }
                        QGCButton {
                            width:      ScreenTools.defaultFontPixelWidth * 10
                            text:       qsTr("OK")
                            enabled:    _userText.text !== "" && _passText.text !== ""
                            onClicked:  QGroundControl.linkManager.connectToAirLinkServer(_userText.text, _passText.text)
                        }
                        QGCButton {
                            width:      ScreenTools.defaultFontPixelWidth * 10
                            text:       qsTr("Cancel")
                            onClicked: {
                                settingsLoader.sourceComponent = null
                                QGroundControl.linkManager.cancelConfigurationEditing(settingsLoader.editingConfig)
                            }
                        }
                    }
                }
            }
        }
    }
}
