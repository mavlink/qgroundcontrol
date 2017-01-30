import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

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

                readonly property string _firmwareLabel:    qsTr("Firmware:")
                readonly property string _vehicleLabel:     qsTr("Vehicle:")

                QGCPalette { id: qgcPal }

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    QGCLabel {
                        text: qsTr("Planned Home Position")
                    }

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         1
                        color:          qgcPal.text
                    }

                    Repeater {
                        model: missionItem.textFieldFacts

                        Item {
                            width:  valuesColumn.width
                            height: textField.height

                            QGCLabel {
                                id:                 textFieldLabel
                                anchors.baseline:   textField.baseline
                                text:               object.name
                            }

                            FactTextField {
                                id:             textField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                showUnits:      true
                                fact:           object
                                visible:        !_root.readOnly
                            }

                            FactLabel {
                                anchors.baseline:   textFieldLabel.baseline
                                anchors.right:      parent.right
                                fact:               object
                                visible:            _root.readOnly
                            }
                        }
                    }

                    QGCLabel {
                        width:          parent.width
                        wrapMode:       Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        text:           qsTr("Actual position set by vehicle at flight time.")
                        horizontalAlignment: Text.AlignHCenter
                    }

                    QGCLabel {
                        text:           qsTr("Vehicle Info:")
                        visible:        _multipleFirmware
                    }

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         1
                        color:          qgcPal.text
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
                        }
                        FactComboBox {
                            Layout.fillWidth:   true
                            fact:               QGroundControl.offlineEditingFirmwareType
                            indexModel:         false
                            visible:            _showOfflineEditingCombos
                        }

                        QGCLabel {
                            text:       _firmwareLabel
                            visible:    !_showOfflineEditingCombos
                        }
                        QGCLabel {
                            text:       _missionVehicle.firmwareTypeString
                            visible:    !_showOfflineEditingCombos
                        }

                        QGCLabel {
                            text:       _vehicleLabel
                            visible:    _showOfflineEditingCombos
                        }
                        FactComboBox {
                            id:                 offlineVehicleCombo
                            Layout.fillWidth:   true
                            fact:               QGroundControl.offlineEditingVehicleType
                            indexModel:         false
                            visible:            _showOfflineEditingCombos
                        }

                        QGCLabel {
                            text:       _vehicleLabel
                            visible:    !_showOfflineEditingCombos
                        }
                        QGCLabel {
                            text:       _missionVehicle.vehicleTypeString
                            visible:    !_showOfflineEditingCombos
                        }
                        QGCLabel {
                            Layout.row: 2
                            text:       qsTr("Cruise speed:")
                            visible:    _showCruiseSpeed
                        }
                        FactTextField {
                            Layout.fillWidth:   true
                            fact:               QGroundControl.offlineEditingCruiseSpeed
                            visible:            _showCruiseSpeed
                        }

                        QGCLabel {
                            Layout.row: 3
                            text:       qsTr("Hover speed:")
                            visible:    _showHoverSpeed
                        }
                        FactTextField {
                            Layout.fillWidth:   true
                            fact:               QGroundControl.offlineEditingHoverSpeed
                            visible:            _showHoverSpeed
                        }
                    } // GridLayout

                    QGCLabel {
                        width:          parent.width
                        wrapMode:       Text.WordWrap
                        font.pointSize: ScreenTools.smallFontPointSize
                        visible:        _multipleFirmware
                        text:           qsTr("Speeds are only for time calculations. Actual vehicle will not be affected.")
                    }

                    Rectangle {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        height:         1
                        color:          qgcPal.text
                    }

                    QGCButton {
                        text:       qsTr("Set Home To Map Center")
                        onClicked:  editorRoot.moveHomeToMapCenter()
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                } // Column
            } // Item
        } // Component
    } // Loader
} // Rectangle
