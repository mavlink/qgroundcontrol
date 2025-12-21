import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for Mission Settings
Rectangle {
    id: root
    width: availableWidth
    height: valuesColumn.height + (_margin * 2)
    color: qgcPal.windowShadeDark
    radius: ScreenTools.defaultBorderRadius

    property var _masterController: masterController
    property var _missionController: _masterController.missionController
    property var _controllerVehicle: _masterController.controllerVehicle
    property bool _vehicleHasHomePosition: _controllerVehicle.homePosition.isValid
    property bool _showCruiseSpeed: !_controllerVehicle.multiRotor
    property bool _showHoverSpeed: _controllerVehicle.multiRotor || _controllerVehicle.vtol
    property bool _multipleFirmware: !QGroundControl.singleFirmwareSupport
    property bool _multipleVehicleTypes: !QGroundControl.singleVehicleSupport
    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 16
    property bool _mobile: ScreenTools.isMobile
    property var _savePath: QGroundControl.settingsManager.appSettings.missionSavePath
    property var _appSettings: QGroundControl.settingsManager.appSettings
    property bool _waypointsOnlyMode: QGroundControl.corePlugin.options.missionWaypointsOnly
    property bool _showCameraSection: (_waypointsOnlyMode || QGroundControl.corePlugin.showAdvancedUI) && !_controllerVehicle.apmFirmware
    property bool _simpleMissionStart: QGroundControl.corePlugin.options.showSimpleMissionStart
    property bool _showFlightSpeed: !_controllerVehicle.vtol && !_simpleMissionStart && !_controllerVehicle.apmFirmware
    property bool _allowFWVehicleTypeSelection: _noMissionItemsAdded && !globals.activeVehicle

    readonly property string _firmwareLabel: qsTr("Firmware")
    readonly property string _vehicleLabel: qsTr("Vehicle")
    readonly property real _margin: ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id: qgcPal }
    Component { id: altModeDialogComponent; AltModeDialog { } }

    Connections {
        target: _controllerVehicle
        function onSupportsTerrainFrameChanged() {
            if (!_controllerVehicle.supportsTerrainFrame && _missionController.globalAltitudeMode === QGroundControl.AltitudeModeTerrainFrame) {
                _missionController.globalAltitudeMode = QGroundControl.AltitudeModeCalcAboveTerrain
            }
        }
    }

    ColumnLayout {
        id: valuesColumn
        anchors.margins: _margin
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: ScreenTools.defaultFontPixelHeight / 2

        LabelledButton {
            Layout.fillWidth: true
            label: qsTr("Altitude Mode")
            buttonText: QGroundControl.altitudeModeShortDescription(_missionController.globalAltitudeMode)

            onClicked: {
                var removeModes = []
                var updateFunction = function(altMode){ _missionController.globalAltitudeMode = altMode }
                if (!_controllerVehicle.supportsTerrainFrame) {
                    removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                }
                if (!_noMissionItemsAdded) {
                    if (_missionController.globalAltitudeMode !== QGroundControl.AltitudeModeRelative) {
                        removeModes.push(QGroundControl.AltitudeModeRelative)
                    }
                    if (_missionController.globalAltitudeMode !== QGroundControl.AltitudeModeAbsolute) {
                        removeModes.push(QGroundControl.AltitudeModeAbsolute)
                    }
                    if (_missionController.globalAltitudeMode !== QGroundControl.AltitudeModeCalcAboveTerrain) {
                        removeModes.push(QGroundControl.AltitudeModeCalcAboveTerrain)
                    }
                    if (_missionController.globalAltitudeMode !== QGroundControl.AltitudeModeTerrainFrame) {
                        removeModes.push(QGroundControl.AltitudeModeTerrainFrame)
                    }
                }
                altModeDialogComponent.createObject(mainWindow, { rgRemoveModes: removeModes, updateAltModeFn: updateFunction }).open()
            }
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: qsTr("Waypoints Altitude")
            fact: QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude
        }

        FactTextFieldSlider {
            Layout.fillWidth: true
            label: qsTr("Flight Speed")
            fact: missionItem.speedSection.flightSpeed
            showEnableCheckbox: true
            enableCheckBoxChecked: missionItem.speedSection.specifyFlightSpeed
            visible: missionItem.speedSection.available

            onEnableCheckboxClicked: missionItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
        }

        /*
        Removed for now. May come back...
        SectionHeader {
            id: createFromTemplateSection
            Layout.fillWidth: true
            text: qsTr("Create Plan From Template")
            checked: true
            visible: !_masterController.containsItems && !_masterController.manualCreation
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: ScreenTools.defaultFontPixelWidth * 0.75
            rowSpacing: columnSpacing
            visible: createFromTemplateSection.visible && createFromTemplateSection.checked

            Repeater {
                model: _masterController.planCreators

                Rectangle {
                    id: planCreatorButton
                    Layout.fillWidth: true
                    implicitHeight: planCreatorLayout.implicitHeight
                    color: planCreatorButtonMouseArea.pressed || planCreatorButtonMouseArea.containsMouse ? qgcPal.buttonHighlight : qgcPal.button

                    ColumnLayout {
                        id: planCreatorLayout
                        width: parent.width
                        spacing: 0

                        Image {
                            id: planCreatorImage
                            Layout.fillWidth: true
                            source: object.imageResource
                            sourceSize.width: width
                            fillMode: Image.PreserveAspectFit
                            mipmap: true
                        }

                        QGCLabel {
                            id: planCreatorNameLabel
                            Layout.fillWidth: true
                            Layout.maximumWidth: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: object.name
                            color: planCreatorButtonMouseArea.pressed || planCreatorButtonMouseArea.containsMouse ? qgcPal.buttonHighlightText : qgcPal.buttonText
                        }
                    }

                    QGCMouseArea {
                        id: planCreatorButtonMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        preventStealing: true
                        onClicked: {
                            if (object.blankPlan) {
                                _masterController.manualCreation = true
                            } else {
                                object.createPlan(_mapCenter())
                            }
                        }

                        function _mapCenter() {
                            var centerPoint = Qt.point(editorMap.centerViewport.left + (editorMap.centerViewport.width / 2), editorMap.centerViewport.top + (editorMap.centerViewport.height / 2))
                            return editorMap.toCoordinate(centerPoint, false)
                        }
                    }
                }
            }
        }
        */

        Column {
            Layout.fillWidth: true
            spacing: _margin
            visible: !_simpleMissionStart

            CameraSection {
                id: cameraSection
                visible: _showCameraSection

                Component.onCompleted: checked = !_waypointsOnlyMode && missionItem.cameraSection.settingsSpecified
            }

            QGCLabel {
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Above camera commands will take affect immediately upon mission start.")
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                font.pointSize: ScreenTools.smallFontPointSize
                visible: _showCameraSection && cameraSection.checked
            }

            SectionHeader {
                id: vehicleInfoSectionHeader
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Vehicle Info")
                visible: !_waypointsOnlyMode
                checked: false
            }

            GridLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                columnSpacing: ScreenTools.defaultFontPixelWidth
                rowSpacing: columnSpacing
                columns: 2
                visible: vehicleInfoSectionHeader.visible && vehicleInfoSectionHeader.checked

                QGCLabel {
                    text: _firmwareLabel
                    Layout.fillWidth: true
                    visible: _multipleFirmware
                }
                FactComboBox {
                    fact: QGroundControl.settingsManager.appSettings.offlineEditingFirmwareClass
                    indexModel: false
                    Layout.preferredWidth: _fieldWidth
                    visible: _multipleFirmware && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text: _controllerVehicle.firmwareTypeString
                    visible: _multipleFirmware && !_allowFWVehicleTypeSelection
                }

                QGCLabel {
                    text: _vehicleLabel
                    Layout.fillWidth: true
                    visible: _multipleVehicleTypes
                }
                FactComboBox {
                    fact: QGroundControl.settingsManager.appSettings.offlineEditingVehicleClass
                    indexModel: false
                    Layout.preferredWidth: _fieldWidth
                    visible: _multipleVehicleTypes && _allowFWVehicleTypeSelection
                }
                QGCLabel {
                    text: _controllerVehicle.vehicleTypeString
                    visible: _multipleVehicleTypes && !_allowFWVehicleTypeSelection
                }

                QGCLabel {
                    Layout.columnSpan: 2
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize
                    text: qsTr("The following speed values are used to calculate total mission time. They do not affect the flight speed for the mission.")
                    visible: _showCruiseSpeed || _showHoverSpeed
                }

                QGCLabel {
                    text: qsTr("Cruise speed")
                    visible: _showCruiseSpeed
                    Layout.fillWidth: true
                }
                FactTextField {
                    fact: QGroundControl.settingsManager.appSettings.offlineEditingCruiseSpeed
                    visible: _showCruiseSpeed
                    Layout.preferredWidth: _fieldWidth
                }

                QGCLabel {
                    text: qsTr("Hover speed")
                    visible: _showHoverSpeed
                    Layout.fillWidth: true
                }
                FactTextField {
                    fact: QGroundControl.settingsManager.appSettings.offlineEditingHoverSpeed
                    visible: _showHoverSpeed
                    Layout.preferredWidth: _fieldWidth
                }
            } // GridLayout

            SectionHeader {
                id: plannedHomePositionSection
                anchors.left: parent.left
                anchors.right: parent.right
                text: qsTr("Launch Position")
                visible: !_vehicleHasHomePosition
                checked: false
            }

            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: _margin
                visible: plannedHomePositionSection.checked && !_vehicleHasHomePosition

                GridLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    columnSpacing: ScreenTools.defaultFontPixelWidth
                    rowSpacing: columnSpacing
                    columns: 2

                    QGCLabel {
                        text: qsTr("Altitude")
                    }
                    FactTextField {
                        fact: missionItem.plannedHomePositionAltitude
                        Layout.fillWidth: true
                    }
                }

                QGCLabel {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize
                    text: qsTr("Actual position set by vehicle at flight time.")
                    horizontalAlignment: Text.AlignHCenter
                }

                QGCButton {
                    text: qsTr("Set To Map Center")
                    onClicked: missionItem.coordinate = map.center
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        } // Column
    } // Column
} // Rectangle
