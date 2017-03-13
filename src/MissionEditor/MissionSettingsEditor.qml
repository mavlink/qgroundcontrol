import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Vehicle           1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0
import QGroundControl.SettingsManager   1.0

// Editor for Mission Settings
Rectangle {
    id:                 valuesRect
    width:              availableWidth
    height:             deferedload.status == Loader.Ready ? (visible ? deferedload.item.height : 0) : 0
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    Loader {
        id:              deferedload
        active:          valuesRect.visible
        asynchronous:    true
        anchors.margins: _margin
        anchors.left:    valuesRect.left
        anchors.right:   valuesRect.right
        anchors.top:     valuesRect.top

        sourceComponent: Component {
            Item {
                id:                 valuesItem
                height:             valuesColumn.height + (_margin * 2)

                property var    _missionVehicle:            missionController.vehicle
                property bool   _offlineEditing:            _missionVehicle.isOfflineEditingVehicle
                property bool   _showOfflineEditingCombos:  _offlineEditing && _noMissionItemsAdded
                property bool   _showCruiseSpeed:           !_missionVehicle.multiRotor
                property bool   _showHoverSpeed:            _missionVehicle.multiRotor || missionController.vehicle.vtol
                property bool   _multipleFirmware:          QGroundControl.supportedFirmwareCount > 2
                property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 16
                property bool   _mobile:                    ScreenTools.isMobile

                readonly property string _firmwareLabel:    qsTr("Firmware")
                readonly property string _vehicleLabel:     qsTr("Vehicle")

                QGCPalette { id: qgcPal }

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    SectionHeader { text: qsTr("Planned Home Position") }

                    Repeater {
                        model: missionItem.textFieldFacts
                        RowLayout {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            spacing:        _margin
                            visible:        _mobile ? index == 2 : true // Cheating here since we known we only have Lat/Lon/Alt

                            QGCLabel { text: object.name; Layout.fillWidth: true }
                            FactTextField {
                                Layout.preferredWidth:  _fieldWidth
                                showUnits:  true
                                fact:       object
                                visible:    !_root.readOnly
                            }
                            FactLabel {
                                Layout.preferredWidth:  _fieldWidth
                                fact:       object
                                visible:    _root.readOnly
                            }
                        }
                    }

                    QGCLabel {
                        width:                  parent.width
                        wrapMode:               Text.WordWrap
                        font.pointSize:         ScreenTools.smallFontPointSize
                        text:                   qsTr("Actual position set by vehicle at flight time.")
                        horizontalAlignment:    Text.AlignHCenter
                    }

                    QGCButton {
                        text:                       qsTr("Set Home To Map Center")
                        onClicked:                  editorRoot.moveHomeToMapCenter()
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }

                    Item {
                        height:     _sectionSpacer
                        width:      1
                        visible:    !ScreenTools.isTinyScreen
                    }

                    SectionHeader {
                        text:           qsTr("Vehicle Info")
                        visible:        _multipleFirmware
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     columnSpacing
                        columns:        2
                        visible:        _multipleFirmware

                        QGCLabel {
                            text:       _firmwareLabel
                            visible:    _showOfflineEditingCombos
                            Layout.fillWidth: true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.appSettings.offlineEditingFirmwareType
                            indexModel:         false
                            visible:            _showOfflineEditingCombos
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:       _vehicleLabel
                            visible:    _showOfflineEditingCombos
                            Layout.fillWidth: true
                        }
                        FactComboBox {
                            id:                 offlineVehicleCombo
                            fact:               QGroundControl.settingsManager.appSettings.offlineEditingVehicleType
                            indexModel:         false
                            visible:            _showOfflineEditingCombos
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:       qsTr("Cruise speed")
                            visible:    _showCruiseSpeed
                            Layout.fillWidth: true
                        }
                        FactTextField {
                            fact:               QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                            visible:            _showCruiseSpeed
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:       qsTr("Hover speed")
                            visible:    _showHoverSpeed
                            Layout.fillWidth: true
                        }
                        FactTextField {
                            fact:               QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                            visible:            _showHoverSpeed
                            Layout.preferredWidth:  _fieldWidth
                        }
                    } // GridLayout

                    RowLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        _margin
                        visible:        !_multipleFirmware
                        QGCLabel { text: qsTr("Hover speed"); Layout.fillWidth: true }
                        FactTextField {
                            Layout.preferredWidth:  _fieldWidth
                            fact:       QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                        }
                    }

                    QGCLabel {
                        width:          parent.width
                        wrapMode:       Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        text:           qsTr("Speeds are only used for time calculations. Actual vehicle speed will not be affected.")
                        horizontalAlignment: Text.AlignHCenter
                    }
                } // Column
            } // Item
        } // Component
    } // Loader
} // Rectangle
