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
    height:             valuesColumn.height + (_margin * 2)
    color:              qgcPal.windowShadeDark
    visible:            missionItem.isCurrentItem
    radius:             _radius

    property var    _masterControler:               masterController
    property var    _missionController:             _masterControler.missionController
    property var    _controllerVehicle:             _masterControler.controllerVehicle
    property bool   _vehicleHasHomePosition:        _controllerVehicle.homePosition.isValid
    property bool   _showCruiseSpeed:               !_controllerVehicle.multiRotor
    property bool   _showHoverSpeed:                _controllerVehicle.multiRotor || _controllerVehicle.vtol
    property bool   _multipleFirmware:              !QGroundControl.singleFirmwareSupport
    property bool   _multipleVehicleTypes:          !QGroundControl.singleVehicleSupport
    property real   _fieldWidth:                    ScreenTools.defaultFontPixelWidth * 16
    property bool   _mobile:                        ScreenTools.isMobile
    property var    _savePath:                      QGroundControl.settingsManager.appSettings.missionSavePath
    property var    _fileExtension:                 QGroundControl.settingsManager.appSettings.missionFileExtension
    property var    _appSettings:                   QGroundControl.settingsManager.appSettings
    property bool   _waypointsOnlyMode:             QGroundControl.corePlugin.options.missionWaypointsOnly
    property bool   _showCameraSection:             (_waypointsOnlyMode || QGroundControl.corePlugin.showAdvancedUI) && !_controllerVehicle.apmFirmware
    property bool   _simpleMissionStart:            QGroundControl.corePlugin.options.showSimpleMissionStart
    property bool   _showFlightSpeed:               !_controllerVehicle.vtol && !_simpleMissionStart && !_controllerVehicle.apmFirmware
    property bool   _allowFWVehicleTypeSelection:   _noMissionItemsAdded && !globals.activeVehicle

    readonly property string _firmwareLabel:    qsTr("Firmware")
    readonly property string _vehicleLabel:     qsTr("Vehicle")
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }
    QGCFileDialogController { id: fileController }

    ColumnLayout {
        id:                 valuesColumn
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        parent.top
        spacing:            _margin

        QGCLabel {
            text:           qsTr("All Altitudes")
            font.pointSize: ScreenTools.smallFontPointSize
        }
        QGCComboBox {
            id:                     altModeCombo
            model:                  enumStrings
            Layout.fillWidth:       true
            enabled:                _noMissionItemsAdded
            onActivated:            _missionController.globalAltitudeMode = enumValues[index]
            Component.onCompleted:  buildEnumStrings()

            readonly property var enumStringsBase:   [ QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeRelative),
                QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAbsolute),
                QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeAboveTerrain),
                QGroundControl.altitudeModeShortDescription(QGroundControl.AltitudeModeTerrainFrame),
                qsTr("Mixed Modes") ]
            readonly property var enumValuesBase:    [QGroundControl.AltitudeModeRelative, QGroundControl.AltitudeModeAbsolute, QGroundControl.AltitudeModeAboveTerrain, QGroundControl.AltitudeModeTerrainFrame, QGroundControl.AltitudeModeNone  ]

            property var enumStrings:   [ ]
            property var enumValues:    [ ]

            function buildEnumStrings() {
                var newEnumStrings = enumStringsBase.slice(0)
                var newEnumValues = enumValuesBase.slice(0)
                if (!_controllerVehicle.supportsTerrainFrame) {
                    // We need to find and pull out the QGroundControl.AltitudeModeTerrainFrame values
                    var deleteIndex = newEnumValues.lastIndexOf(QGroundControl.AltitudeModeTerrainFrame)
                    newEnumStrings.splice(deleteIndex, 1)
                    newEnumValues.splice(deleteIndex, 1)
                    if (_missionController.globalAltitudeMode == QGroundControl.AltitudeModeTerrainFrame) {
                        _missionController.globalAltitudeMode = QGroundControl.AltitudeModeAboveTerrain
                    }
                }
                enumStrings = newEnumStrings
                enumValues = newEnumValues
                currentIndex = enumValues.lastIndexOf(_missionController.globalAltitudeMode)
            }

            Connections {
                target:                         _controllerVehicle
                onSupportsTerrainFrameChanged:  altModeCombo.buildEnumStrings()
            }
        }
        QGCLabel {
            Layout.fillWidth:       true
            wrapMode:               Text.WordWrap
            horizontalAlignment:    Text.AlignHCenter
            font.pointSize:         ScreenTools.smallFontPointSize
            text:                   switch(_missionController.globalAltitudeMode) {
                                    case QGroundControl.AltitudeModeAboveTerrain:
                                        qsTr("Specified altitudes are distance above terrain. Actual altitudes sent to vehicle are calculated from terrain data and sent in AMSL")
                                        break
                                    case QGroundControl.AltitudeModeTerrainFrame:
                                        qsTr("Specified altitudes are distance above terrain. The actual altitude flown is controlled by the vehicle either from terrain height maps being sent to vehicle or a distance sensor.")
                                        break
                                    case QGroundControl.AltitudeModeNone:
                                        qsTr("The altitude mode can differ for each individual item.")
                                        break
                                    default:
                                        ""
                                        break
                                    }
            visible:                _missionController.globalAltitudeMode == QGroundControl.AltitudeModeAboveTerrain || _missionController.globalAltitudeMode == QGroundControl.AltitudeModeTerrainFrame || _missionController.globalAltitudeMode == QGroundControl.AltitudeModeNone
        }

        QGCLabel {
            text:           qsTr("Initial Waypoint Alt")
            font.pointSize: ScreenTools.smallFontPointSize
        }
        AltitudeFactTextField {
            fact:               QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
            altitudeMode:       _missionController.globalAltitudeModeDefault
            Layout.fillWidth:   true
        }

        GridLayout {
            Layout.fillWidth:   true
            columnSpacing:      ScreenTools.defaultFontPixelWidth
            rowSpacing:         columnSpacing
            columns:            2

            QGCCheckBox {
                id:         flightSpeedCheckBox
                text:       qsTr("Flight speed")
                visible:    _showFlightSpeed
                checked:    missionItem.speedSection.specifyFlightSpeed
                onClicked:   missionItem.speedSection.specifyFlightSpeed = checked
            }
            FactTextField {
                Layout.fillWidth:   true
                fact:               missionItem.speedSection.flightSpeed
                visible:            _showFlightSpeed
                enabled:            flightSpeedCheckBox.checked
            }
        }

        Column {
            Layout.fillWidth:   true
            spacing:            _margin
            visible:            !_simpleMissionStart

            CameraSection {
                id:         cameraSection
                checked:    !_waypointsOnlyMode && missionItem.cameraSection.settingsSpecified
                visible:    _showCameraSection
            }

            QGCLabel {
                anchors.left:           parent.left
                anchors.right:          parent.right
                text:                   qsTr("Above camera commands will take affect immediately upon mission start.")
                wrapMode:               Text.WordWrap
                horizontalAlignment:    Text.AlignHCenter
                font.pointSize:         ScreenTools.smallFontPointSize
                visible:                _showCameraSection && cameraSection.checked
            }

            SectionHeader {
                id:             vehicleInfoSectionHeader
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           qsTr("Vehicle Info")
                visible:        !_waypointsOnlyMode
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
                    visible:            _multipleFirmware
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                    indexModel:             false
                    Layout.preferredWidth:  _fieldWidth
                    visible:                _multipleFirmware && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text:       _controllerVehicle.firmwareTypeString
                    visible:    _multipleFirmware && !_allowFWVehicleTypeSelection
                }

                QGCLabel {
                    text:               _vehicleLabel
                    Layout.fillWidth:   true
                    visible:            _multipleVehicleTypes
                }
                FactComboBox {
                    fact:                   QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                    indexModel:             false
                    Layout.preferredWidth:  _fieldWidth
                    visible:                _multipleVehicleTypes && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text:       _controllerVehicle.vehicleTypeString
                    visible:    _multipleVehicleTypes && !_allowFWVehicleTypeSelection
                }

                QGCLabel {
                    Layout.columnSpan:      2
                    Layout.alignment:       Qt.AlignHCenter
                    Layout.fillWidth:       true
                    wrapMode:               Text.WordWrap
                    font.pointSize:         ScreenTools.smallFontPointSize
                    text:                   qsTr("The following speed values are used to calculate total mission time. They do not affect the flight speed for the mission.")
                    visible:                _showCruiseSpeed || _showHoverSpeed
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
                id:             plannedHomePositionSection
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           qsTr("Launch Position")
                visible:        !_vehicleHasHomePosition
                checked:        false
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
                    text:                       qsTr("Set To Map Center")
                    onClicked:                  missionItem.coordinate = map.center
                    anchors.horizontalCenter:   parent.horizontalCenter
                }
            }
        } // Column
    } // Column
} // Rectangle
