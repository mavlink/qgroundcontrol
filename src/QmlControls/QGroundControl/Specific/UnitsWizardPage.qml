import QtQuick 2.12
import QtQuick.Layouts 1.12

import QGroundControl 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.SettingsManager 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Specific 1.0

BaseStartupWizardPage {
    width: settingsColumn.width
    height: settingsColumn.height

    property real _margins: ScreenTools.defaultFontPixelWidth
    property real _comboFieldWidth: ScreenTools.defaultFontPixelWidth * 20

    doneText: qsTr("Confirm")

    ColumnLayout {
        id:                         settingsColumn
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight

        QGCLabel {
            id:         unitsSectionLabel
            text:       qsTr("Choose the measurement units you want to use in the application. You can change it later on in General Settings.")

            Layout.preferredWidth: unitsGrid.width
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.preferredHeight: unitsGrid.height + (_margins * 2)
            Layout.preferredWidth:  unitsGrid.width + (_margins * 2)
            color:                  qgcPal.windowShade
            Layout.fillWidth:       true

            GridLayout {
                id:                         unitsGrid
                anchors.topMargin:          _margins
                anchors.top:                parent.top
                Layout.fillWidth:           false
                anchors.horizontalCenter:   parent.horizontalCenter
                flow:                       GridLayout.TopToBottom
                rows:                       5

                QGCLabel { text: qsTr("System of units") }

                Repeater {
                    model: [ qsTr("Distance"), qsTr("Area"), qsTr("Speed"), qsTr("Temperature") ]
                    QGCLabel { text: modelData }
                }

                QGCComboBox {
                    model: [qsTr("Metric System"), qsTr("Imperial System")]
                    Layout.preferredWidth:  _comboFieldWidth

                    currentIndex: QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsMeters ? 0 : 1

                    onActivated: {
                        var metric = (currentIndex === 0);
                        QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits.value = metric ? UnitsSettings.HorizontalDistanceUnitsMeters : UnitsSettings.HorizontalDistanceUnitsFeet
                        QGroundControl.settingsManager.unitsSettings.areaUnits.value = metric ? UnitsSettings.AreaUnitsSquareMeters : UnitsSettings.AreaUnitsSquareFeet
                        QGroundControl.settingsManager.unitsSettings.speedUnits.value = metric ? UnitsSettings.SpeedUnitsMetersPerSecond : UnitsSettings.SpeedUnitsFeetPerSecond
                        QGroundControl.settingsManager.unitsSettings.temperatureUnits.value = metric ? UnitsSettings.TemperatureUnitsCelsius : UnitsSettings.TemperatureUnitsFarenheit
                    }
                }
                Repeater {
                    model:  [ QGroundControl.settingsManager.unitsSettings.horizontalDistanceUnits, QGroundControl.settingsManager.unitsSettings.areaUnits, QGroundControl.settingsManager.unitsSettings.speedUnits, QGroundControl.settingsManager.unitsSettings.temperatureUnits ]
                    FactComboBox {
                        Layout.preferredWidth:  _comboFieldWidth
                        fact:                   modelData
                        indexModel:             false
                    }
                }
            }
        }
    }
}
