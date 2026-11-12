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
    /// The accuracy in the application's horizontal distance unit. The unit's name is a property, so the text follows
    /// a change of the unit setting; the conversion that returns both in one string is a method call, which no
    /// binding follows.
    readonly property string _horizontalAccuracyText: {
        const conversion = QGroundControl.unitsConversion
        const accuracy = Number(conversion.metersToAppSettingsHorizontalDistanceUnits(root.horizontalAccuracy))
        return accuracy.toFixed(1) + " " + conversion.appSettingsHorizontalDistanceUnitsString
    }

    heading: qsTr("Ground Station Position")

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
        labelText: root._horizontalAccuracyText
        labelTextFormat: Text.PlainText
    }
}
