/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs  1.1

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:                 _linkRoot
    color:              __qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var _currentSelection: null
    property int _firstColumn:      ScreenTools.defaultFontPixelWidth * 12
    property int _secondColumn:     ScreenTools.defaultFontPixelWidth * 30

    ExclusiveGroup { id: linkGroup }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    function openCommSettings(lconf) {
        settingLoader.linkConfig = lconf
        settingLoader.sourceComponent = commSettings
        settingLoader.visible = true
    }

    function closeCommSettings() {
        settingLoader.visible = false
        settingLoader.sourceComponent = null
    }

    Flickable {
        clip:               true
        anchors.top:        parent.top
        width:              parent.width
        height:             parent.height - buttonRow.height
        contentHeight:      settingsColumn.height
        contentWidth:       _linkRoot.width
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior:     Flickable.StopAtBounds

        Column {
            id:                 settingsColumn
            width:              _linkRoot.width
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                text:   "Comm Link Settings (WIP)"
                font.pixelSize: ScreenTools.mediumFontPixelSize
            }
            Rectangle {
                height: 1
                width:  parent.width
                color:  qgcPal.button
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Repeater {
                model: QGroundControl.linkManager.linkConfigurations
                delegate:
                QGCButton {
                    text:   object.name
                    width:  _linkRoot.width * 0.5
                    exclusiveGroup: linkGroup
                    anchors.horizontalCenter: settingsColumn.horizontalCenter
                    onClicked: {
                        checked = true
                        _currentSelection = object
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
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Delete"
            enabled:    _currentSelection && !_currentSelection.dynamic
            onClicked: {
                if(_currentSelection)
                    deleteDialog.visible = true
            }
            MessageDialog {
                id:         deleteDialog
                visible:    false
                icon:       StandardIcon.Warning
                standardButtons: StandardButton.Yes | StandardButton.No
                title:      "Remove Link Configuration"
                text:       _currentSelection ? "Remove " + _currentSelection.name + ". Is this really what you want?" : ""
                onYes: {
                    if(_currentSelection)
                        QGroundControl.linkManager.removeConfiguration(_currentSelection)
                    deleteDialog.visible = false
                }
                onNo: {
                    deleteDialog.visible = false
                }
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Edit"
            enabled:    _currentSelection && !_currentSelection.link
            onClicked: {
                _linkRoot.openCommSettings(_currentSelection)
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Add"
            onClicked: {
                _linkRoot.openCommSettings(null)
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Connect"
            enabled:    _currentSelection && !_currentSelection.link
            onClicked: {
                QGroundControl.linkManager.createConnectedLink(_currentSelection)
                settingsMenu.closeSettings()
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Disconnect"
            enabled:    _currentSelection && _currentSelection.link
            onClicked: {
                QGroundControl.linkManager.disconnectLink(_currentSelection.link, false)
            }
        }
    }

    Loader {
        id:             settingLoader
        anchors.fill:   parent
        visible:        false
        property var linkConfig: null
        property var editConfig: null
    }

    //---------------------------------------------
    // Comm Settings
    Component {
        id: commSettings
        Rectangle {
            color:          __qgcPal.window
            anchors.fill:   parent
            Component.onCompleted: {
                // If editing, create copy for editing
                if(linkConfig) {
                    editConfig = QGroundControl.linkManager.startConfigurationEditing(linkConfig)
                } else {
                    // Create new link configuration
                    if(ScreenTools.isiOS)
                        editConfig = QGroundControl.linkManager.createConfiguration(LinkConfiguration.TypeUdp, "Unnamed")
                    else
                        editConfig = QGroundControl.linkManager.createConfiguration(LinkConfiguration.TypeSerial, "Unnamed")
                }
            }
            Component.onDestruction: {
                if(editConfig) {
                    QGroundControl.linkManager.cancelConfigurationEditing(editConfig)
                    editConfig = null
                }
            }
            Flickable {
                clip:               true
                anchors.top:        parent.top
                width:              parent.width
                height:             parent.height - commButtonRow.height
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                contentHeight:      commSettingsColumn.height
                contentWidth:       _linkRoot.width
                flickableDirection: Flickable.VerticalFlick
                boundsBehavior:     Flickable.StopAtBounds
                Column {
                    id:                 commSettingsColumn
                    width:              _linkRoot.width
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    spacing:            ScreenTools.defaultFontPixelHeight / 2
                    QGCLabel {
                        text:   linkConfig ? "Edit Link Configuration Settings (WIP)" : "Create New Link Configuration (WIP)"
                        font.pixelSize: ScreenTools.mediumFontPixelSize
                    }
                    Rectangle {
                        height: 1
                        width:  parent.width
                        color:  qgcPal.button
                    }
                    Item {
                        height: ScreenTools.defaultFontPixelHeight / 2
                        width:  parent.width
                    }
                    Row {
                        spacing:    ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:   "Name:"
                            width:  _firstColumn
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCTextField {
                            id:     nameField
                            text:   editConfig ? editConfig.name : ""
                            width:  _secondColumn
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:       "Type:"
                            width:      _firstColumn
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        //-----------------------------------------------------
                        // When editing, you can't change the link type
                        QGCLabel {
                            text:       linkConfig ? QGroundControl.linkManager.linkTypeStrings[linkConfig.linkType] : ""
                            visible:    linkConfig != null
                            width:      _secondColumn
                            anchors.verticalCenter: parent.verticalCenter
                            Component.onCompleted: {
                                if(linkConfig != null) {
                                    var index = linkConfig.linkType
                                    if(index === LinkConfiguration.TypeSerial)
                                        linkSettingLoader.sourceComponent = serialLinkSettings
                                    if(index === LinkConfiguration.TypeUdp)
                                        linkSettingLoader.source = "UdpSettings.qml"
                                    if(index === LinkConfiguration.TypeTcp)
                                        linkSettingLoader.sourceComponent = tcpLinkSettings
                                    if(index === LinkConfiguration.TypeMock)
                                        linkSettingLoader.sourceComponent = mockLinkSettings
                                    if(index === LinkConfiguration.TypeLogReplay)
                                        linkSettingLoader.sourceComponent = logLinkSettings
                                    linkSettingLoader.visible = true
                                }
                            }
                        }
                        //-----------------------------------------------------
                        // When creating, select a link type
                        QGCComboBox {
                            id:             linkTypeCombo
                            width:          _secondColumn
                            visible:        linkConfig == null
                            model:          QGroundControl.linkManager.linkTypeStrings
                            anchors.verticalCenter: parent.verticalCenter
                            onActivated: {
                                if (index != -1 && index !== editConfig.linkType) {
                                    // Destroy current panel
                                    linkSettingLoader.sourceComponent = null
                                    linkSettingLoader.source = ""
                                    linkSettingLoader.visible = false
                                    // Save current name
                                    var name = editConfig.name
                                    // Discard link configuration (old type)
                                    QGroundControl.linkManager.cancelConfigurationEditing(editConfig)
                                    // Create new link configuration
                                    editConfig = QGroundControl.linkManager.createConfiguration(index, name)
                                    // Load appropriate configuration panel
                                    if(index === LinkConfiguration.TypeSerial)
                                        linkSettingLoader.sourceComponent = serialLinkSettings
                                    if(index === LinkConfiguration.TypeUdp)
                                        linkSettingLoader.source = "UdpSettings.qml"
                                    if(index === LinkConfiguration.TypeTcp)
                                        linkSettingLoader.sourceComponent = tcpLinkSettings
                                    if(index === LinkConfiguration.TypeMock)
                                        linkSettingLoader.sourceComponent = mockLinkSettings
                                    if(index === LinkConfiguration.TypeLogReplay)
                                        linkSettingLoader.sourceComponent = logLinkSettings
                                    linkSettingLoader.visible = true
                                }
                            }
                            Component.onCompleted: {
                                if(linkConfig == null) {
                                    linkTypeCombo.currentIndex = 0
                                    var index = editConfig.linkType
                                    if(index === LinkConfiguration.TypeSerial)
                                        linkSettingLoader.sourceComponent = serialLinkSettings
                                    if(index === LinkConfiguration.TypeUdp)
                                        linkSettingLoader.source = "UdpSettings.qml"
                                    if(index === LinkConfiguration.TypeTcp)
                                        linkSettingLoader.sourceComponent = tcpLinkSettings
                                    if(index === LinkConfiguration.TypeMock)
                                        linkSettingLoader.sourceComponent = mockLinkSettings
                                    if(index === LinkConfiguration.TypeLogReplay)
                                        linkSettingLoader.sourceComponent = logLinkSettings
                                    linkSettingLoader.visible = true
                                }
                            }
                        }
                    }
                    Item {
                        height: ScreenTools.defaultFontPixelHeight * 0.5
                        width:  parent.width
                    }
                    //-- Auto Connect
                    QGCCheckBox {
                        text:       "Automatically Connect on Start"
                        checked:    false
                        enabled:    editConfig ? editConfig.autoConnectAllowed : false
                        onCheckedChanged: {
                            if(editConfig) {
                                editConfig.autoConnect = checked
                            }
                        }
                        Component.onCompleted: {
                            if(editConfig)
                                checked = editConfig.autoConnect
                        }
                    }
                    Item {
                        height: ScreenTools.defaultFontPixelHeight
                        width:  parent.width
                    }
                    Loader {
                        id:             linkSettingLoader
                        width:          parent.width
                        visible:        false
                        property var subEditConfig: editConfig
                    }
                }
            }
            Row {
                id:                 commButtonRow
                spacing:            ScreenTools.defaultFontPixelWidth
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.bottom:     parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    width:      ScreenTools.defaultFontPixelWidth * 10
                    text:       "OK"
                    enabled:    nameField.text !== ""
                    onClicked: {
                        // Save editting
                        editConfig.name = nameField.text
                        if(linkConfig) {
                            QGroundControl.linkManager.endConfigurationEditing(linkConfig, editConfig)
                        } else {
                            // If it was edited, it's no longer "dynamic"
                            editConfig.dynamic = false
                            QGroundControl.linkManager.endCreateConfiguration(editConfig)
                        }
                        editConfig = null
                        _linkRoot.closeCommSettings()
                    }
                }
                QGCButton {
                    width:      ScreenTools.defaultFontPixelWidth * 10
                    text:       "Cancel"
                    onClicked: {
                        QGroundControl.linkManager.cancelConfigurationEditing(editConfig)
                        editConfig = null
                        _linkRoot.closeCommSettings()
                    }
                }
            }
        }
    }
    //---------------------------------------------
    // Serial Link Settings
    Component {
        id: serialLinkSettings
        Column {
            width:              serialLinkSettings.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                id:     serialLabel
                text:   "Serial Link Settings"
            }
            Rectangle {
                height: 1
                width:  serialLabel.width
                color:  qgcPal.button
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Serial Port:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    id:             commPortCombo
                    width:          _secondColumn
                    model:          QGroundControl.linkManager.serialPortStrings
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: {
                        if (index != -1) {
                            subEditConfig.portName = QGroundControl.linkManager.serialPorts[index]
                        }
                    }
                    Component.onCompleted: {
                        if(subEditConfig != null) {
                            if(subEditConfig.portDisplayName === "")
                                subEditConfig.portName = QGroundControl.linkManager.serialPorts[0]
                            var index = commPortCombo.find(subEditConfig.portDisplayName)
                            if (index === -1) {
                                console.warn("Serial Port not present", subEditConfig.portName)
                            } else {
                                commPortCombo.currentIndex = index
                            }
                        } else {
                            commPortCombo.currentIndex = 0
                        }
                    }
                }
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Baud Rate:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    id:             baudCombo
                    width:          _secondColumn
                    model:          QGroundControl.linkManager.serialBaudRates
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: {
                        if (index != -1) {
                            subEditConfig.baud = parseInt(QGroundControl.linkManager.serialBaudRates[index])
                        }
                    }
                    Component.onCompleted: {
                        var baud = "57600"
                        if(subEditConfig != null) {
                            baud = subEditConfig.baud.toString()
                        }
                        var index = baudCombo.find(baud)
                        if (index === -1) {
                            console.warn("Baud rate name not in combo box", baud)
                        } else {
                            baudCombo.currentIndex = index
                        }
                    }
                }
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            //-----------------------------------------------------------------
            //-- Advanced Serial Settings
            QGCCheckBox {
                id:     showAdvanced
                text:   "Show Advanced Serial Settings"
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            //-- Flow Control
            QGCCheckBox {
                text:       "Enable Flow Control"
                checked:    subEditConfig ? subEditConfig.flowControl !== 0 : false
                visible:    showAdvanced.checked
                onCheckedChanged: {
                    if(subEditConfig) {
                        subEditConfig.flowControl = checked ? 1 : 0
                    }
                }
            }
            //-- Parity
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                visible:    showAdvanced.checked
                QGCLabel {
                    text:   "Parity:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    id:             parityCombo
                    width:          _firstColumn
                    model:          ["None", "Even", "Odd"]
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: {
                        if (index != -1) {
                            // Hard coded values from qserialport.h
                            if(index == 0)
                                subEditConfig.parity = 0
                            else if(index == 1)
                                subEditConfig.parity = 2
                            else
                                subEditConfig.parity = 3
                        }
                    }
                    Component.onCompleted: {
                        var index = 0
                        if(subEditConfig != null) {
                            index = subEditConfig.parity
                        }
                        if(index > 1) {
                            index = index - 2
                        }
                        parityCombo.currentIndex = index
                    }
                }
            }
            //-- Data Bits
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                visible:    showAdvanced.checked
                QGCLabel {
                    text:   "Data Bits:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    id:             dataCombo
                    width:          _firstColumn
                    model:          ["5", "6", "7", "8"]
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: {
                        if (index != -1) {
                            subEditConfig.dataBits = index + 5
                        }
                    }
                    Component.onCompleted: {
                        var index = 3
                        if(subEditConfig != null) {
                            index = subEditConfig.parity - 5
                            if(index < 0)
                                index = 3
                        }
                        dataCombo.currentIndex = index
                    }
                }
            }
            //-- Stop Bits
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                visible:    showAdvanced.checked
                QGCLabel {
                    text:   "Stop Bits:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    id:             stopCombo
                    width:          _firstColumn
                    model:          ["1", "2"]
                    anchors.verticalCenter: parent.verticalCenter
                    onActivated: {
                        if (index != -1) {
                            subEditConfig.stopBits = index + 1
                        }
                    }
                    Component.onCompleted: {
                        var index = 0
                        if(subEditConfig != null) {
                            index = subEditConfig.stopBits - 1
                            if(index < 0)
                                index = 0
                        }
                        stopCombo.currentIndex = index
                    }
                }
            }
        }
    }
    //---------------------------------------------
    // TCP Link Settings
    Component {
        id: tcpLinkSettings
        Column {
            width:              tcpLinkSettings.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                id:     tcpLabel
                text:   "TCP Link Settings"
            }
            Rectangle {
                height: 1
                width:  tcpLabel.width
                color:  qgcPal.button
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Host Address:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:     hostField
                    text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcp ? subEditConfig.host : ""
                    width:  _secondColumn
                    anchors.verticalCenter: parent.verticalCenter
                    onTextChanged: {
                        if(subEditConfig) {
                            subEditConfig.host = hostField.text
                        }
                    }
                }
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "TCP Port:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:     portField
                    text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcp ? subEditConfig.port.toString() : ""
                    width:  _firstColumn
                    inputMethodHints:       Qt.ImhFormattedNumbersOnly
                    anchors.verticalCenter: parent.verticalCenter
                    onTextChanged: {
                        if(subEditConfig) {
                            subEditConfig.port = parseInt(portField.text)
                        }
                    }
                }
            }
        }
    }
    //---------------------------------------------
    // Log Replay Settings
    Component {
        id: logLinkSettings
        Column {
            width:              logLinkSettings.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                text:   "Log Replay Link Settings"
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Log File:"
                    width:  _firstColumn
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:     logField
                    text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeMock ? subEditConfig.fileName : ""
                    width:  _secondColumn
                    anchors.verticalCenter: parent.verticalCenter
                    onTextChanged: {
                        if(subEditConfig) {
                            subEditConfig.filename = logField.text
                        }
                    }
                }
                QGCButton {
                    text:   "Browse"
                    onClicked: {
                        fileDialog.visible = true
                    }
                }
            }
            FileDialog {
                id:         fileDialog
                title:      "Please choose a file"
                folder:     shortcuts.home
                visible:    false
                selectExisting: true
                onAccepted: {
                    if(subEditConfig) {
                        subEditConfig.fileName = fileDialog.fileUrl.toString().replace("file://", "")
                    }
                    fileDialog.visible = false
                }
                onRejected: {
                    fileDialog.visible = false
                }
            }
        }
    }
    //---------------------------------------------
    // Mock Link Settings
    Component {
        id: mockLinkSettings
        Column {
            width:              mockLinkSettings.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                text:   "Mock Link Settings"
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
        }
    }
}
