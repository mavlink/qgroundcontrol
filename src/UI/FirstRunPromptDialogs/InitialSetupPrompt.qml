import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

FirstRunPrompt {
    title: qsTr("Preferences")
    promptId: QGroundControl.corePlugin.initialSetupPromptId

    property var _appSettings: QGroundControl.settingsManager.appSettings
    property var _unitsSettings: QGroundControl.settingsManager.unitsSettings
    property bool _multipleFirmware: !QGroundControl.singleFirmwareSupport
    property bool _multipleVehicleTypes: !QGroundControl.singleVehicleSupport
    property var _rgFacts: [ _unitsSettings.horizontalDistanceUnits, _unitsSettings.verticalDistanceUnits, _unitsSettings.areaUnits, _unitsSettings.speedUnits, _unitsSettings.temperatureUnits ]
    property var _rgLabels: [ qsTr("Horizontal Distance"), qsTr("Vertical Distance"), qsTr("Area"), qsTr("Speed"), qsTr("Temperature") ]

    function changeSystemOfUnits(metric) {
        unitComboBoxRepeater.model = 0
        unitComboBoxRepeater.model = _rgFacts.length

        if (_unitsSettings.horizontalDistanceUnits.userVisible) {
            _unitsSettings.horizontalDistanceUnits.value = metric ? UnitsSettings.HorizontalDistanceUnitsMeters : UnitsSettings.HorizontalDistanceUnitsFeet
        }
        if (_unitsSettings.verticalDistanceUnits.userVisible) {
            _unitsSettings.verticalDistanceUnits.value = metric ? UnitsSettings.VerticalDistanceUnitsMeters : UnitsSettings.VerticalDistanceUnitsFeet
        }
        if (_unitsSettings.areaUnits.userVisible) {
            _unitsSettings.areaUnits.value = metric ? UnitsSettings.AreaUnitsSquareMeters : UnitsSettings.AreaUnitsSquareFeet
        }
        if (_unitsSettings.speedUnits.userVisible) {
            _unitsSettings.speedUnits.value = metric ? UnitsSettings.SpeedUnitsMetersPerSecond : UnitsSettings.SpeedUnitsFeetPerSecond
        }
        if (_unitsSettings.temperatureUnits.userVisible) {
            _unitsSettings.temperatureUnits.value = metric ? UnitsSettings.TemperatureUnitsCelsius : UnitsSettings.TemperatureUnitsFarenheit
        }
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Vehicle Preferences")
            headingDescription: qsTr("Select the firmware and vehicle type you use. This tailors the interface for a simpler experience. Choose 'All' if you use multiple types.")

            LabelledFactComboBox {
                Layout.fillWidth: true
                label: qsTr("Preferred Firmware")
                fact: _appSettings.preferredFirmwareClass
                indexModel: false
                visible: _multipleFirmware
            }

            LabelledFactComboBox {
                Layout.fillWidth: true
                label: qsTr("Preferred Vehicle")
                fact: _appSettings.preferredVehicleClass
                indexModel: false
                visible: _multipleVehicleTypes
            }

            LabelledFactComboBox {
                Layout.fillWidth: true
                label: qsTr("Offline Firmware")
                fact: _appSettings.offlineEditingFirmwareClass
                indexModel: false
                visible: _multipleFirmware && _appSettings.preferredFirmwareClass.value === 0
            }

            LabelledFactComboBox {
                Layout.fillWidth: true
                label: qsTr("Offline Vehicle")
                fact: _appSettings.offlineEditingVehicleClass
                indexModel: false
                visible: _multipleVehicleTypes && _appSettings.preferredVehicleClass.value === 0
            }
        }

        SettingsGroupLayout {
            Layout.fillWidth: true
            heading: qsTr("Measurement Units")
            headingDescription: qsTr("Choose the measurement units you want to use. You can also change it later in General Settings.")

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth * 2

                QGCLabel {
                    Layout.fillWidth: true
                    text: qsTr("System of units")
                }

                QGCComboBox {
                    sizeToContents: true
                    model: [ qsTr("Metric System"), qsTr("Imperial System") ]
                    currentIndex: _unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsMeters ? 0 : 1
                    onActivated: (index) => { changeSystemOfUnits(currentIndex === 0) }
                }
            }

            Repeater {
                id: unitComboBoxRepeater
                model: _rgFacts.length

                LabelledFactComboBox {
                    Layout.fillWidth: true
                    label: _rgLabels[index]
                    fact: _rgFacts[index]
                    indexModel: false
                    visible: _rgFacts[index].userVisible
                }
            }
        }
    }
}
