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
import QGroundControl.Controllers       1.0

// Editor for Mission Settings
Rectangle {
    id:                 valuesRect
    width:              availableWidth
    height:             deferedload.status == Loader.Ready ? (visible ? deferedload.item.height : 0) : 0
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    property var    _missionVehicle:            missionController.vehicle
    property bool   _vehicleHasHomePosition:    _missionVehicle.homePosition.isValid
    property bool   _offlineEditing:            _missionVehicle.isOfflineEditingVehicle
    property bool   _showOfflineVehicleCombos:  _offlineEditing && _multipleFirmware && _noMissionItemsAdded
    property bool   _showCruiseSpeed:           !_missionVehicle.multiRotor
    property bool   _showHoverSpeed:            _missionVehicle.multiRotor || _missionVehicle.vtol
    property bool   _multipleFirmware:          QGroundControl.supportedFirmwareCount > 2
    property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 16
    property bool   _mobile:                    ScreenTools.isMobile
    property var    _savePath:                  QGroundControl.settingsManager.appSettings.missionSavePath
    property var    _fileExtension:             QGroundControl.settingsManager.appSettings.missionFileExtension
    property bool   _newMissionAlreadyExists:   false
    property bool   _noMissionName:             missionItem.missionName.valueString === ""
    property bool   _showMissionList:           _noMissionItemsAdded && (_noMissionName || _newMissionAlreadyExists)
    property bool   _existingMissionLoaded:     missionItem.existingMission
    property var    _appSettings:               QGroundControl.settingsManager.appSettings

    readonly property string _firmwareLabel:    qsTr("Firmware")
    readonly property string _vehicleLabel:     qsTr("Vehicle")

    QGCPalette { id: qgcPal }
    QFileDialogController { id: fileController }

    Connections {
        target:             missionItem.missionName

        onRawValueChanged: {
            if (!_existingMissionLoaded) {
                _newMissionAlreadyExists = !_noMissionName && fileController.fileExists(_savePath + "/" + missionItem.missionName.valueString + "." + _fileExtension)
            }
        }
    }

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

                Column {
                    id:             valuesColumn
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    spacing:        _margin

                    QGCLabel {
                        text:           qsTr("Mission name")
                        font.pointSize: ScreenTools.smallFontPointSize
                    }

                    FactTextField {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        fact:           missionItem.missionName
                        visible:        !_existingMissionLoaded
                    }

                    QGCLabel {
                        text:       missionItem.missionName.valueString
                        visible:    _existingMissionLoaded
                    }

                    QGCLabel {
                        text:           qsTr("Mission already exists")
                        font.pointSize: ScreenTools.smallFontPointSize
                        color:          qgcPal.warningText
                        visible:        !_existingMissionLoaded && _newMissionAlreadyExists
                    }

                    FactCheckBox {
                        id:     automaticUploadCheckbox
                        text:   qsTr("Automatically upload on exit")
                        fact:   _appSettings.automaticMissionUpload
                    }

                    RowLayout {
                        anchors.left:   parent.left
                        anchors.right:  parent.right

                        QGCButton {
                            text:               qsTr("Clear")
                            visible:            !_noMissionItemsAdded
                            Layout.fillWidth:   true
                            onClicked:          missionController.clearMission()
                        }

                        QGCButton {
                            text:               qsTr("Close")
                            visible:            !_noMissionItemsAdded
                            Layout.fillWidth:   true
                            onClicked:          missionController.closeMission()
                        }

                        QGCButton {
                            text:               qsTr("Upload")
                            visible:            !_noMissionItemsAdded && !automaticUploadCheckbox.checked
                            Layout.fillWidth:   true
                            onClicked:          missionController.sendToVehicle()
                        }
                    }

                    Loader {
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        sourceComponent:    _showMissionList ? missionList : missionSettings
                    }
                } // Column
            } // Item
        } // Component
    } // Loader

    Component {
        id: missionList

        QGCFlickable {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         missionColumn.height

            Column {
                id:             missionColumn
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                SectionHeader {
                    text:       qsTr("Load Mission")
                    showSpacer: false
                }

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right

                    QGCButton {
                        text:               qsTr("From Vehicle")
                        visible:            !_offlineEditing
                        Layout.fillWidth:   true
                        onClicked:          missionController.loadFromVehicle()
                    }

                    QGCButton {
                        text:               qsTr("Browse")
                        Layout.fillWidth:   true
                        onClicked:          fileDialog.openForLoad()

                        QGCFileDialog {
                            id:             fileDialog
                            qgcView:        rootQgcView
                            title:          qsTr("Select mission file")
                            selectExisting: true
                            folder:         _appSettings.missionSavePath
                            fileExtension:  _appSettings.missionFileExtension
                            nameFilters:    [ qsTr("Mission Files (*.%1)").arg(fileExtension) , qsTr("All Files (*.*)") ]

                            onAcceptedForLoad: {
                                missionController.loadFromFile(file)
                                fileDialog.close()
                            }
                        }
                    }
                }

                QGCLabel {
                    text:       qsTr("No mission files")
                    visible:    missionRepeater.model.length === 0
                }

                Repeater {
                    id:     missionRepeater
                    model:  fileController.getFiles(_savePath, _fileExtension);

                    QGCButton {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        text:           modelData

                        onClicked: {
                            missionController.loadFromFile(fileController.fullyQualifiedFilename(_savePath, modelData, _fileExtension))
                        }
                    }
                }
            }
        }
    }

    Component {
        id: missionSettings

        Column {
            id:             valuesColumn
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.top:    parent.top
            spacing:        _margin

            SectionHeader {
                id:         missionDefaultsSectionHeader
                text:       qsTr("Mission Defaults")
                checked:    true
                showSpacer: false
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
                        text:       qsTr("Waypoint alt")
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

                FactComboBox {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    fact:           missionItem.missionEndAction
                    indexModel:     false
                }
            }

            CameraSection {
                checked: missionItem.cameraSection.settingsSpecified
            }

            SectionHeader {
                id:         vehicleInfoSectionHeader
                text:       qsTr("Vehicle Info")
                visible:    _offlineEditing
                checked:    false
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
                    visible:            _showOfflineVehicleCombos
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingFirmwareType
                    indexModel:             false
                    Layout.preferredWidth:  _fieldWidth
                    visible:                _showOfflineVehicleCombos
                }

                QGCLabel {
                    text:               _vehicleLabel
                    Layout.fillWidth:   true
                    visible:            _showOfflineVehicleCombos
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingVehicleType
                    indexModel:             false
                    Layout.preferredWidth:  _fieldWidth
                    visible:                _showOfflineVehicleCombos
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
                id:         plannedHomePositionSection
                text:       qsTr("Planned Home Position")
                visible:    !_vehicleHasHomePosition
                checked:    false
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                visible:        plannedHomePositionSection.checked && !_vehicleHasHomePosition

                GridLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    columnSpacing:  ScreenTools.defaultFontPixelWidth
                    rowSpacing:     columnSpacing
                    columns:        2

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
        } // Column
    } // Deferred loader
} // Rectangle
