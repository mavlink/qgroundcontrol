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
                property bool   _showHoverSpeed:            _missionVehicle.multiRotor || _missionVehicle.vtol
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

                    SectionHeader {
                        id:             plannedHomePositionSection
                        text:           qsTr("Planned Home Position")
                        showSpacer:     false
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        _margin
                        visible:        plannedHomePositionSection.checked

                        GridLayout {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            columnSpacing:  ScreenTools.defaultFontPixelWidth
                            rowSpacing:     columnSpacing
                            columns:        2

                            QGCLabel {
                                text: qsTr("Latitude")
                            }
                            FactTextField {
                                fact:               missionItem.plannedHomePositionLatitude
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text: qsTr("Longitude")
                            }
                            FactTextField {
                                fact:               missionItem.plannedHomePositionLongitude
                                Layout.fillWidth:   true
                            }

                            QGCLabel {
                                text: qsTr("Altitude")
                            }
                            FactTextField {
                                fact:               missionItem.plannedHomePositionAltitude
                                Layout.fillWidth:   true
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
                            onClicked:                  missionItem.coordinate = map.center
                            anchors.horizontalCenter:   parent.horizontalCenter
                        }
                    }

                    SectionHeader {
                        id:             vehicleInfoSectionHeader
                        text:           qsTr("Vehicle Info")
                        visible:        _multipleFirmware && _showOfflineEditingCombos
                        checked:        false
                    }

                    GridLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        columnSpacing:  ScreenTools.defaultFontPixelWidth
                        rowSpacing:     columnSpacing
                        columns:        2
                        visible:        vehicleInfoSectionHeader.visible && vehicleInfoSectionHeader.checked

                        QGCLabel {
                            text:               _firmwareLabel
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.appSettings.offlineEditingFirmwareType
                            indexModel:             false
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:               _vehicleLabel
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:                   QGroundControl.settingsManager.appSettings.offlineEditingVehicleType
                            indexModel:             false
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:               qsTr("Cruise speed")
                            visible:            _showCruiseSpeed
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:                   QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                            visible:                _showCruiseSpeed
                            Layout.preferredWidth:  _fieldWidth
                        }

                        QGCLabel {
                            text:               qsTr("Hover speed")
                            visible:            _showHoverSpeed
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:                   QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                            visible:                _showHoverSpeed
                            Layout.preferredWidth:  _fieldWidth
                        }
                    } // GridLayout

                    SectionHeader {
                        id:             missionDefaultsSectionHeader
                        text:           qsTr("Mission Defaults")
                        checked:        false
                    }

                    Column {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        spacing:        _margin
                        visible:        missionDefaultsSectionHeader.checked

                        GridLayout {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            columnSpacing:  ScreenTools.defaultFontPixelWidth
                            rowSpacing:     columnSpacing
                            columns:        2

                            QGCLabel {
                                text:       qsTr("Altitude")
                            }
                            FactTextField {
                                fact:               QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
                                Layout.fillWidth:   true
                            }

                            QGCCheckBox {
                                id:         flightSpeedCheckBox
                                text:       qsTr("Flight speed")
                                visible:    !_missionVehicle.vtol
                                checked:    missionItem.specifyMissionFlightSpeed
                                onClicked:  missionItem.specifyMissionFlightSpeed = checked
                            }
                            FactTextField {
                                Layout.fillWidth:   true
                                fact:               missionItem.missionFlightSpeed
                                visible:            flightSpeedCheckBox.visible
                                enabled:            flightSpeedCheckBox.checked
                            }
                        } // GridLayout

                        /*
                          FIXME: NYI
                        FactComboBox {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            fact:           missionItem.missionEndAction
                            indexModel:     false
                        }
                        */
                    }

                    CameraSection {
                        checked: missionItem.cameraSection.settingsSpecified
                    }

                    QGCLabel {
                        width:                  parent.width
                        wrapMode:               Text.WordWrap
                        font.pointSize:         ScreenTools.smallFontPointSize
                        text:                   qsTr("Speeds are only used for time calculations. Actual vehicle speed will not be affected.")
                        horizontalAlignment:    Text.AlignHCenter
                        visible:                _offlineEditing && _missionVehicle.vtol
                    }
                } // Column
            } // Item
        } // Component
    } // Loader
} // Rectangle
