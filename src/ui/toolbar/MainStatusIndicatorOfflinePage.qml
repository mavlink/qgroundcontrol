/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0

ToolIndicatorPage {
    showExpand: true

    property var    linkConfigs:            QGroundControl.linkManager.linkConfigurations
    property bool   noLinks:                true
    property var    editingConfig:          null
    property var    autoConnectSettings:    QGroundControl.settingsManager.autoConnectSettings

    Component.onCompleted: {
        for (var i = 0; i < linkConfigs.count; i++) {
            var linkConfig = linkConfigs.get(i)
            if (!linkConfig.dynamic && !linkConfig.isAutoConnect) {
                noLinks = false
                break
            }
        }
    }

    contentComponent: Component {
            ColumnLayout { 
            spacing: ScreenTools.defaultFontPixelHeight / 2

            QGCLabel {
                Layout.alignment:   Qt.AlignTop
                text:               noLinks ? qsTr("No Links Configured") : qsTr("Connect To Link")
                font.pointSize:     noLinks ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize
            }
            
            Repeater {
                model: linkConfigs

                delegate: QGCButton {
                    Layout.fillWidth:   true
                    text:               object.name + (object.link ? " (" + qsTr("Connected") + ")" : "")
                    visible:            !object.dynamic
                    enabled:            !object.link
                    autoExclusive:      true

                    onClicked: {
                        QGroundControl.linkManager.createConnectedLink(object)
                        drawer.close()
                    }
                }
            }
        }
    }

    expandedComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            IndicatorPageGroupLayout {
                RowLayout {
                    QGCLabel { Layout.fillWidth: true; text: qsTr("Communication Links") }
                    
                    QGCButton {
                        text:       qsTr("Configure")
                        onClicked: {
                            mainWindow.showSettingsTool(qsTr("Comm Links"))
                            drawer.close()
                        }
                    }
                }
            }

            IndicatorPageGroupLayout {
                heading:        qsTr("AutoConnect")
                visible:        autoConnectSettings.visible
                showDivider:    false

                Repeater {
                    id: autoConnectRepeater

                    model: [ 
                        autoConnectSettings.autoConnectPixhawk,
                        autoConnectSettings.autoConnectSiKRadio,
                        autoConnectSettings.autoConnectPX4Flow,
                        autoConnectSettings.autoConnectLibrePilot,
                        autoConnectSettings.autoConnectUDP,
                        autoConnectSettings.autoConnectZeroConf,
                    ]

                    property var names: [ qsTr("Pixhawk"), qsTr("SiK Radio"), qsTr("PX4 Flow"), qsTr("LibrePilot"), qsTr("UDP"), qsTr("Zero-Conf") ]

                    FactCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               autoConnectRepeater.names[index]
                        fact:               modelData
                        visible:            modelData.visible
                    }
                }
            }
        }
    }
}
