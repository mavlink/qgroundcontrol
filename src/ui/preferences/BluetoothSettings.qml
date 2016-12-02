/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs  1.1

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    id:     _btSettings
    width:  parent ? parent.width : 0
    height: btColumn.height

    function saveSettings() {
        // No need
    }

    QGCLabel {
        text:       qsTr("Bluetooth Not Available")
        visible:    !QGroundControl.linkManager.isBluetoothAvailable
        anchors.centerIn: parent
    }

    Column {
        id:         btColumn
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        visible:    QGroundControl.linkManager.isBluetoothAvailable

        ExclusiveGroup { id: linkGroup }

        QGCPalette {
            id:                 qgcPal
            colorGroupEnabled:  enabled
        }

        QGCLabel {
            id:     btLabel
            text:   qsTr("Bluetooth Link Settings")
        }
        Rectangle {
            height: 1
            width:  btLabel.width
            color:  qgcPal.button
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        Row {
            spacing:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   qsTr("Device:")
                width:  _firstColumn
            }
            QGCLabel {
                id:     deviceField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth ? subEditConfig.devName : ""
            }
        }
        Row {
            visible:    !ScreenTools.isiOS
            spacing:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   qsTr("Address:")
                width:  _firstColumn
            }
            QGCLabel {
                id:     addressField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth ? subEditConfig.address : ""
            }
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        QGCLabel {
            text:   qsTr("Bluetooth Devices:")
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
                        visible: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth && subEditConfig.nameList.length > 0
                    }
                    Repeater {
                        model: subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth ? subEditConfig.nameList : ""
                        delegate:
                        QGCButton {
                            text:   modelData
                            width:  _secondColumn
                            anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
                            exclusiveGroup: linkGroup
                            onClicked: {
                                checked = true
                                if(subEditConfig && modelData !== "")
                                    subEditConfig.devName = modelData
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
                                text:       qsTr("Scan")
                                enabled:    subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth && !subEditConfig.scanning
                                onClicked: {
                                    if(subEditConfig)
                                        subEditConfig.startScan()
                                }
                            }
                            QGCButton {
                                width:      ScreenTools.defaultFontPixelWidth * 10
                                text:       qsTr("Stop")
                                enabled:    subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeBluetooth && subEditConfig.scanning
                                onClicked: {
                                    if(subEditConfig)
                                        subEditConfig.stopScan()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
