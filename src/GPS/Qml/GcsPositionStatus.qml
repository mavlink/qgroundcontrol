pragma ComponentBehavior: Bound

import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading: qsTr("GCS Position")

    /// Offer the source selector; read-only summaries show only the source in use.
    property bool sourceEditable: true
    property bool showCoordinates: true

    property geoCoordinate gcsPosition: root._positionManager.gcsPosition
    /// In meters; unknown when negative or not finite.
    property real horizontalAccuracy: root._positionManager.gcsPositionHorizontalAccuracy

    readonly property PositionManager _positionManager: QGroundControl.positionManager

    LabelledFactComboBox {
        objectName: "gcsPositionSource"
        visible: root.sourceEditable
        label: qsTr("Source")
        fact: QGroundControl.settingsManager.rtkSettings.gcsPositionSource
    }

    LabelledLabel {
        objectName: "gcsPositionSelectedSource"
        label:     qsTr("Using")
        labelText: root._positionManager.selectedSourceName
    }

    LabelledLabel {
        objectName: "gcsPositionSourceStatus"
        label:     qsTr("Status")
        labelText: root._positionManager.sourceStatusText
    }

    LabelledLabel {
        visible:   root.showCoordinates && root.gcsPosition.isValid
        label:     qsTr("Latitude")
        labelText: root.gcsPosition.latitude.toFixed(7)
    }

    LabelledLabel {
        visible:   root.showCoordinates && root.gcsPosition.isValid
        label:     qsTr("Longitude")
        labelText: root.gcsPosition.longitude.toFixed(7)
    }

    LabelledLabel {
        objectName: "gcsHorizontalAccuracy"
        visible:   root.showCoordinates && root.gcsPosition.isValid
                   && Number.isFinite(root.horizontalAccuracy) && root.horizontalAccuracy >= 0
        label:     qsTr("Horizontal accuracy")
        labelText: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(root.horizontalAccuracy, 1)
    }
}
