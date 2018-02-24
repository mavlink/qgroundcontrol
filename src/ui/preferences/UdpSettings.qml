/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

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
        QGCCheckBox {
            text:       "High Latency"
            checked:    false
            visible:    editConfig ? editConfig.highLatencyAllowed : false
            onCheckedChanged: {
                if(editConfig) {
                    editConfig.highLatency = checked
                }
            }
            Component.onCompleted: {
                if(editConfig)
                    checked = editConfig.highLatency
            }
        }
    }
}
