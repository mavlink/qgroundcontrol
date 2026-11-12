pragma ComponentBehavior: Bound

import QtQuick
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

QGCGroupBox {
    id: root

    /// Offer the source selector; read-only summaries show only the source in use.
    property bool sourceEditable: true
    property bool showCoordinates: true

    property geoCoordinate gcsPosition: root._positionManager.gcsPosition
    /// In meters; unknown when negative or not finite.
    property real horizontalAccuracy: root._positionManager.gcsPositionHorizontalAccuracy

    readonly property PositionManager _positionManager: QGroundControl.positionManager

    heading: qsTr("GCS Position")

    LabelledFactComboBox {
        objectName: "gcsPositionSource"
        visible: root.sourceEditable
        label: qsTr("Source")
        fact: QGroundControl.settingsManager.rtkSettings.gcsPositionSource
    }

    // The source and its status are words and a sentence, which wrap where the box is narrower than they are, as in
    // the GPS indicator's status column.
    GPSWrappingRow {
        objectName: "gcsPositionSelectedSource"
        label:     qsTr("Using")
        labelText: root._positionManager.selectedSourceName
        labelWrapMode: Text.Wrap
    }

    GPSWrappingRow {
        objectName: "gcsPositionSourceStatus"
        label:     qsTr("Status")
        labelText: root._positionManager.sourceStatusText
        labelWrapMode: Text.Wrap
    }

    LabelledLabel {
        visible:   root.showCoordinates && root.gcsPosition.isValid
        label:     qsTr("Latitude")
        labelText: root.gcsPosition.latitude.toFixed(7)
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        visible:   root.showCoordinates && root.gcsPosition.isValid
        label:     qsTr("Longitude")
        labelText: root.gcsPosition.longitude.toFixed(7)
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        objectName: "gcsHorizontalAccuracy"
        visible:   root.showCoordinates && root.gcsPosition.isValid
                   && Number.isFinite(root.horizontalAccuracy) && root.horizontalAccuracy >= 0
        label:     qsTr("Horizontal accuracy")
        labelText: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(root.horizontalAccuracy, 1)
        labelTextFormat: Text.PlainText
    }
}
