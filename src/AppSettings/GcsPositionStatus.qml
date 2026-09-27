import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root
    heading: qsTr("GCS Position")

    /// Offer the source selector; read-only summaries show only the source in use.
    property bool sourceEditable: true
    property bool showCoordinates: true

    readonly property var _positionManager: QGroundControl.positionManager
    property var  _gcsPosition: root._positionManager.gcsPosition
    property real _horizontalAccuracy: root._positionManager.gcsPositionHorizontalAccuracy

    LabelledFactComboBox {
        objectName: "gcsPositionSource"
        Layout.fillWidth: true
        visible: root.sourceEditable
        label: qsTr("Source")
        fact: QGroundControl.settingsManager.rtkSettings.gcsPositionSource
    }

    LabelledLabel {
        objectName: "gcsPositionSelectedSource"
        Layout.fillWidth: true
        label:     qsTr("Using")
        labelText: root._positionManager.selectedSourceName
    }

    LabelledLabel {
        objectName: "gcsPositionSourceStatus"
        Layout.fillWidth: true
        label:     qsTr("Status")
        labelText: root._positionManager.sourceStatusText
    }

    LabelledLabel {
        Layout.fillWidth: true
        visible:   root.showCoordinates && root._gcsPosition.isValid
        label:     qsTr("Latitude")
        labelText: root._gcsPosition.isValid ? root._gcsPosition.latitude.toFixed(7) : qsTr("N/A")
    }

    LabelledLabel {
        Layout.fillWidth: true
        visible:   root.showCoordinates && root._gcsPosition.isValid
        label:     qsTr("Longitude")
        labelText: root._gcsPosition.isValid ? root._gcsPosition.longitude.toFixed(7) : qsTr("N/A")
    }

    LabelledLabel {
        Layout.fillWidth: true
        objectName: "gcsHorizontalAccuracy"
        visible:   root.showCoordinates && root._gcsPosition.isValid
        label:     qsTr("Horizontal accuracy")
        labelText: Number.isFinite(root._horizontalAccuracy) && root._horizontalAccuracy >= 0
                   ? qsTr("%1 m").arg(root._horizontalAccuracy.toFixed(1)) : qsTr("N/A")
    }
}
