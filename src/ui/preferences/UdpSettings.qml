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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Column {
    id:                 _udpSetting
    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    function saveSettings() {
        // No need
    }

    property string _currentHost: ""

    Item {
        height: ScreenTools.defaultFontPixelHeight * 2
        width:  ScreenTools.defaultFontPixelWidth * 43
        QGCLabel {
            id:                 warningLabel
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            anchors.right:      parent.right
            font.pointSize:     ScreenTools.smallFontPointSize
            wrapMode:           Text.WordWrap
            text:               qsTr("Note: For best perfomance, please disable AutoConnect to UDP devices on the General page.")
        }
    }

    Row {
        spacing:    ScreenTools.defaultFontPixelWidth
        QGCLabel {
            text:   qsTr("Port:")
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
        width:  ScreenTools.defaultFontPixelWidth * 43
    }
    QGCLabel {
        text:   qsTr("Server addresses (optional):")
    }
    Item {
        width:  hostRow.width
        height: hostRow.height
        Row {
            id:      hostRow
            spacing: ScreenTools.defaultFontPixelWidth
            Column {
                id:         hostColumn
                spacing:    ScreenTools.defaultFontPixelHeight / 2
                Rectangle {
                    height:  1
                    width:   ScreenTools.defaultFontPixelWidth * 43
                    color:   qgcPal.button
                    visible: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeUdp && subEditConfig.hostList.length > 0
                }
                Repeater {
                    model: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeUdp ? subEditConfig.hostList : ""
                    delegate:
                    QGCButton {
                        text:               modelData
                        width:              ScreenTools.defaultFontPixelWidth * 43
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                        autoExclusive:      true
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
                    width:      ScreenTools.defaultFontPixelWidth * 43
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
                    width:  ScreenTools.defaultFontPixelWidth * 43
                    color:  qgcPal.button
                }
                Item {
                    height: ScreenTools.defaultFontPixelHeight / 2
                    width:  ScreenTools.defaultFontPixelWidth * 43
                }
                Item {
                    width:  ScreenTools.defaultFontPixelWidth * 43
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
