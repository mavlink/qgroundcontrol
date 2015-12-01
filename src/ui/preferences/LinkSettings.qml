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
                    anchors.horizontalCenter: parent.horizontalCenter
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
            enabled:    _currentSelection && !_currentSelection.link
            onClicked: {
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
    }

    //---------------------------------------------
    // Comm Settings
    Component {
        id: commSettings
        Rectangle {
            color:          __qgcPal.window
            anchors.fill:   parent
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
                            text:   linkConfig ? linkConfig.name : "Untitled"
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
                                    if(linkConfig.linkType === LinkConfiguration.TypeSerial)
                                        linkSettingLoader.sourceComponent = serialLinkSettings
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
                                if (index != -1) {
                                    linkSettingLoader.sourceComponent = null
                                    if(index === LinkConfiguration.TypeSerial)
                                        linkSettingLoader.sourceComponent = serialLinkSettings
                                    if(index === LinkConfiguration.TypeUdp)
                                        linkSettingLoader.sourceComponent = udpLinkSettings
                                    if(index === LinkConfiguration.TypeTcp)
                                        linkSettingLoader.sourceComponent = tcpLinkSettings
                                    if(index === LinkConfiguration.TypeMock)
                                        linkSettingLoader.sourceComponent = mockLinkSettings
                                    if(index === LinkConfiguration.TypeLogReplay)
                                        linkSettingLoader.sourceComponent = logLinkSettings
                                }
                            }
                            Component.onCompleted: {
                                if(linkConfig == null) {
                                    linkTypeCombo.currentIndex = 0
                                    linkSettingLoader.sourceComponent = serialLinkSettings
                                    linkSettingLoader.visible = true
                                }
                            }
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
                        property var config: linkConfig
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
                    onClicked: {
                        _linkRoot.closeCommSettings()
                    }
                }
                QGCButton {
                    width:      ScreenTools.defaultFontPixelWidth * 10
                    text:       "Cancel"
                    onClicked: {
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
            width:              parent.width
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
                        }
                    }
                    Component.onCompleted: {
                        if(config != null) {
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
                        }
                    }
                    Component.onCompleted: {
                        var baud = "57600"
                        if(config != null) {
                            // Get baud from config
                        }
                        var index = baudCombo.find(baud)
                        if (index == -1) {
                            console.warn("Baud rate name not in combo box", baud)
                        } else {
                            baudCombo.currentIndex = index
                        }
                    }
                }
            }
        }
    }
    //---------------------------------------------
    // UDP Link Settings
    Component {
        id: udpLinkSettings
        Column {
            width:              parent.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                id:     udpLabel
                text:   "UDP Link Settings"
            }
            Rectangle {
                height: 1
                width:  udpLabel.width
                color:  qgcPal.button
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Listening Port:"
                    width:  _firstColumn
                }
                QGCLabel {
                    text:   "14550"
                    width:  _secondColumn
                }
            }
            QGCLabel {
                text:   "Target Hosts:"
            }
        }
    }
    //---------------------------------------------
    // TCP Link Settings
    Component {
        id: tcpLinkSettings
        Column {
            width:              parent.width
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
                    text:   "TCP Port:"
                    width:  _firstColumn
                }
                QGCLabel {
                    text:   "5760"
                    width:  _secondColumn
                }
            }
            Row {
                spacing:    ScreenTools.defaultFontPixelWidth
                QGCLabel {
                    text:   "Host Address:"
                    width:  _firstColumn
                }
                QGCLabel {
                    text:   "0.0.0.0"
                    width:  _secondColumn
                }
            }
        }
    }
    //---------------------------------------------
    // Log Replay Settings
    Component {
        id: logLinkSettings
        Column {
            width:              parent.width
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            QGCLabel {
                text:   "Log Replay Link Settings"
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            QGCButton {
                text:   "Select Log File"
            }
        }
    }
    //---------------------------------------------
    // Mock Link Settings
    Component {
        id: mockLinkSettings
        Column {
            width:              parent.width
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
