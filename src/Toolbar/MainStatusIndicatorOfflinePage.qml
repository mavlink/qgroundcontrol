import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ToolIndicatorPage {
    id:         control
    showExpand: true

    property var    linkConfigs:            QGroundControl.linkManager.linkConfigurations
    property bool   noLinks:                true
    property var    editingConfig:          null
    property var    autoConnectSettings:    QGroundControl.settingsManager.autoConnectSettings
    property Loader preVehicleChecklistLoader:  null
    property bool   showChecklist:          false

    readonly property bool _checklistPassed: !preVehicleChecklistLoader || !preVehicleChecklistLoader.item || preVehicleChecklistLoader.item.allChecksPassed

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
        QGCGroupBox {
            Component.onCompleted: currentIndex = (showChecklist && !_checklistPassed) ? 0 : 1

            QGCGroupBoxTab {
                title:              qsTr("Pre-Flight Checklist")
                buttonObjectName:   "mainStatusChecklistTab"
                tabVisible:         showChecklist
                badge: StatusPill {
                    objectName: "mainStatusChecklistTabBadge"
                    text:       _checklistPassed ? qsTr("Passed") : qsTr("In Progress")
                    color:      _checklistPassed ? QGroundControl.globalPalette.successIndicator : QGroundControl.globalPalette.warningIndicator
                }

                MainStatusChecklistHost {
                    loader: control.preVehicleChecklistLoader
                }
            }

            QGCGroupBoxTab {
                title: qsTr("Select Link to Connect")

                QGCLabel {
                    text:       qsTr("No Links Configured")
                    visible:    noLinks
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
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }
            }
        }
    }

    expandedComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            QGCGroupBox {
                LabelledButton {
                    label:      qsTr("Communication Links")
                    buttonText: qsTr("Configure")

                    onClicked: {
                        // Untranslated page key from SettingsPages.json — do not qsTr()
                        mainWindow.showSettingsTool("Comm Links")
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }

            QGCGroupBox {
                heading:        qsTr("AutoConnect")
                visible:        autoConnectSettings.userVisible

                Repeater {
                    id: autoConnectRepeater

                    model: [
                        autoConnectSettings.autoConnectPixhawk,
                        autoConnectSettings.autoConnectSiKRadio,
                        autoConnectSettings.autoConnectLibrePilot,
                        autoConnectSettings.autoConnectUDP,
                    ]

                    property var names: [ qsTr("Pixhawk"), qsTr("SiK Radio"), qsTr("LibrePilot"), qsTr("UDP") ]

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
