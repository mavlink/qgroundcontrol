import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Map provider, type, and elevation provider selection.
/// Wraps a SettingsGroupLayout with cascading comboboxes driven by QGCMapEngineManager.
SettingsGroupLayout {
    Layout.fillWidth: true

    property var _mapEngineManager:     QGroundControl.mapEngineManager
    property Fact _mapProviderFact:     QGroundControl.settingsManager.flightMapSettings.mapProvider
    property Fact _mapTypeFact:         QGroundControl.settingsManager.flightMapSettings.mapType
    property Fact _elevationProviderFact: QGroundControl.settingsManager.flightMapSettings.elevationMapProvider

    LabelledComboBox {
        label: qsTr("Provider")
        model: _mapEngineManager.mapProviderList

        onActivated: (index) => {
            _mapProviderFact.rawValue = comboBox.textAt(index)
            _mapTypeFact.rawValue = _mapEngineManager.mapTypeList(comboBox.textAt(index))[0]
        }

        Component.onCompleted: {
            var index = comboBox.find(_mapProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Type")
        model: _mapEngineManager.mapTypeList(_mapProviderFact.rawValue)

        onActivated: (index) => { _mapTypeFact.rawValue = comboBox.textAt(index) }

        Component.onCompleted: {
            var index = comboBox.find(_mapTypeFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Elevation Provider")
        model: _mapEngineManager.elevationProviderList

        onActivated: (index) => { _elevationProviderFact.rawValue = comboBox.textAt(index) }

        Component.onCompleted: {
            var index = comboBox.find(_elevationProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }
}
