/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.12
import QtQuick.Layouts 1.12

import QGroundControl 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.SettingsManager 1.0
import QGroundControl.Controls 1.0

FirstRunPrompt {
    title:      qsTr("Measurement Units")
    promptId:   QGroundControl.corePlugin.unitsFirstRunPromptId

    property real   _margins:           ScreenTools.defaultFontPixelHeight / 2
    property var    _unitsSettings:     QGroundControl.settingsManager.unitsSettings
    property var    _rgFacts:           [ _unitsSettings.horizontalDistanceUnits, _unitsSettings.verticalDistanceUnits, _unitsSettings.areaUnits, _unitsSettings.speedUnits, _unitsSettings.temperatureUnits ]
    property var    _rgLabels:          [ qsTr("Horizontal Distance"), qsTr("Vertical Distance"), qsTr("Area"), qsTr("Speed"), qsTr("Temperature") ]
    property int    _cVisibleFacts:     0

    Component.onCompleted: {
        var cVisibleFacts = 0
        for (var i=0; i<_rgFacts.length; i++) {
            if (_rgFacts[i].visible) {
                cVisibleFacts++
            }
        }
        _cVisibleFacts = cVisibleFacts
    }

    function changeSystemOfUnits(metric) {
        if (_unitsSettings.horizontalDistanceUnits.visible) {
            _unitsSettings.horizontalDistanceUnits.value = metric ? UnitsSettings.HorizontalDistanceUnitsMeters : UnitsSettings.HorizontalDistanceUnitsFeet
        }
        if (_unitsSettings.verticalDistanceUnits.visible) {
            _unitsSettings.verticalDistanceUnits.value = metric ? UnitsSettings.VerticalDistanceUnitsMeters : UnitsSettings.VerticalDistanceUnitsFeet
        }
        if (_unitsSettings.areaUnits.visible) {
            _unitsSettings.areaUnits.value = metric ? UnitsSettings.AreaUnitsSquareMeters : UnitsSettings.AreaUnitsSquareFeet
        }
        if (_unitsSettings.speedUnits.visible) {
            _unitsSettings.speedUnits.value = metric ? UnitsSettings.SpeedUnitsMetersPerSecond : UnitsSettings.SpeedUnitsFeetPerSecond
        }
        if (_unitsSettings.temperatureUnits.visible) {
            _unitsSettings.temperatureUnits.value = metric ? UnitsSettings.TemperatureUnitsCelsius : UnitsSettings.TemperatureUnitsFarenheit
        }
    }

    ColumnLayout {
        id:         settingsColumn
        spacing:    ScreenTools.defaultFontPixelHeight

        QGCLabel {
            id:         unitsSectionLabel
            text:       qsTr("Choose the measurement units you want to use. You can also change it later in General Settings.")

            Layout.preferredWidth: unitsGrid.width
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.preferredHeight: unitsGrid.height + (_margins * 2)
            Layout.preferredWidth:  unitsGrid.width + (_margins * 2)
            color:                  qgcPal.windowShade
            Layout.fillWidth:       true

            GridLayout {
                id:                 unitsGrid
                anchors.margins:    _margins
                anchors.top:        parent.top
                anchors.left:       parent.left
                rows:               _cVisibleFacts + 1
                flow:               GridLayout.TopToBottom

                QGCLabel { text: qsTr("System of units") }

                Repeater {
                    model: _rgFacts.length
                    QGCLabel {
                        text:       _rgLabels[index]
                        visible:    _rgFacts[index].visible
                    }
                }

                QGCComboBox {
                    Layout.fillWidth:   true
                    sizeToContents:     true
                    model:              [ qsTr("Metric System"), qsTr("Imperial System") ]
                    currentIndex:       _unitsSettings.horizontalDistanceUnits.value === UnitsSettings.HorizontalDistanceUnitsMeters ? 0 : 1
                    onActivated:        changeSystemOfUnits(currentIndex === 0 /* metric */)
                }

                Repeater {
                    model: _rgFacts.length
                    FactComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        fact:               _rgFacts[index]
                        indexModel:         false
                        visible:            _rgFacts[index].visible
                    }
                }
            }
        }
    }
}
