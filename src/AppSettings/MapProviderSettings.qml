import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Map provider, type, and elevation provider selection.
/// Wraps a QGCGroupBox with cascading comboboxes driven by MapCatalog.
QGCGroupBox {
    Layout.fillWidth: true

    property var _mapCatalog:           QGroundControl.mapCatalog
    property Fact _mapProviderFact:     QGroundControl.settingsManager.flightMapSettings.mapProvider
    property Fact _mapTypeFact:         QGroundControl.settingsManager.flightMapSettings.mapType
    property Fact _elevationProviderFact: QGroundControl.settingsManager.flightMapSettings.elevationMapProvider

    LabelledComboBox {
        label: qsTr("Provider")
        model: _mapCatalog.mapProviderList

        onActivated: (index) => {
            _mapProviderFact.rawValue = comboBox.textAt(index)
            const mapTypes = _mapCatalog.mapTypeList(comboBox.textAt(index))
            if (mapTypes.length > 0) {
                _mapTypeFact.rawValue = mapTypes[0]
            }
        }

        Component.onCompleted: {
            var index = comboBox.find(_mapProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Type")
        model: _mapCatalog.mapTypeList(_mapProviderFact.rawValue)
        // The only map of a provider can have no type
        visible: model.length > 0

        onActivated: (index) => { _mapTypeFact.rawValue = comboBox.textAt(index) }

        Component.onCompleted: {
            var index = comboBox.find(_mapTypeFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }

    LabelledComboBox {
        label: qsTr("Elevation Provider")
        model: _mapCatalog.elevationProviderList

        onActivated: (index) => { _elevationProviderFact.rawValue = comboBox.textAt(index) }

        Component.onCompleted: {
            var index = comboBox.find(_elevationProviderFact.rawValue)
            if (index < 0) index = 0
            comboBox.currentIndex = index
        }
    }
}
