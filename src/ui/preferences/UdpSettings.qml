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

Item {
    id:     _udpSetting
    width:  parent ? parent.width : 0
    height: udpColumn.height

    function saveSettings() {
        // No need
    }

    property var _currentHost: ""

    Column {
        id:         udpColumn
        spacing:    ScreenTools.defaultFontPixelHeight / 2

        ExclusiveGroup { id: linkGroup }

        QGCPalette {
            id:                 qgcPal
            colorGroupEnabled:  enabled
        }

        QGCLabel {
            id:     udpLabel
            text:   qsTr("UDP Link Settings")
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
                text:   qsTr("Listening Port:")
                width:  _firstColumn
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCTextField {
                id:     portField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeUdp ? subEditConfig.localPort.toString() : ""
                focus:  true
                width:  _firstColumn
                inputMethodHints:       Qt.ImhFormattedNumbersOnly
                anchors.verticalCenter: parent.verticalCenter
                onTextChanged: {
                    if(subEditConfig) {
                        subEditConfig.localPort = parseInt(portField.text)
                    }
                }
            }
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        QGCLabel {
            text:   qsTr("Target Hosts:")
        }
        Item {
            width:  hostRow.width
            height: hostRow.height
            Row {
                id:      hostRow
                spacing: ScreenTools.defaultFontPixelWidth
                Item {
                    height: 1
                    width:  _firstColumn
                }
                Column {
                    id:         hostColumn
                    spacing:    ScreenTools.defaultFontPixelHeight / 2
                    Rectangle {
                        height:  1
                        width:   _secondColumn
                        color:   qgcPal.button
                        visible: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeUdp && subEditConfig.hostList.length > 0
                    }
                    Repeater {
                        model: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeUdp ? subEditConfig.hostList : ""
                        delegate:
                        QGCButton {
                            text:   modelData
                            width:  _secondColumn
                            anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                            exclusiveGroup: linkGroup
                            onClicked: {
                                checked = true
                                _udpSetting._currentHost = modelData
                            }
                        }
                    }
                    QGCTextField {
                        id:         hostField
                        focus:      true
                        visible:    false
                        width:      ScreenTools.defaultFontPixelWidth * 30
                        onEditingFinished: {
                            if(subEditConfig) {
                                if(hostField.text !== "") {
                                    subEditConfig.addHost(hostField.text)
                                    hostField.text = ""
                                }
                                hostField.visible = false
                            }
                        }
                        Keys.onReleased: {
                            if (event.key === Qt.Key_Escape) {
                                hostField.text = ""
                                hostField.visible = false
                            }
                        }
                    }
                    Rectangle {
                        height: 1
                        width:  _secondColumn
                        color:  qgcPal.button
                    }
                    Item {
                        height: ScreenTools.defaultFontPixelHeight / 2
                        width:  parent.width
                    }
                    Item {
                        width:  _secondColumn
                        height: udpButtonRow.height
                        Row {
                            id:         udpButtonRow
                            spacing:    ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                text:       qsTr("Add")
                                onClicked: {
                                    if(hostField.visible && hostField.text !== "") {
                                        subEditConfig.addHost(hostField.text)
                                        hostField.text = ""
                                        hostField.visible = false
                                    } else
                                        hostField.visible = true
                                }
                            }
                            QGCButton {
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                enabled:    _udpSetting._currentHost && _udpSetting._currentHost !== ""
                                text:       qsTr("Remove")
                                onClicked: {
                                    subEditConfig.removeHost(_udpSetting._currentHost)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
