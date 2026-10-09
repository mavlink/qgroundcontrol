pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// Live GNSS receiver status, shared by the GPS indicator and the receiver settings page.
SettingsGroupLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// The receiver's status Facts.
    property GPSReceiverFactGroup facts: root.receiver.facts
    /// Keep the group visible without a receiver, showing disconnectedText.
    property bool showWhenDisconnected: false
    property string disconnectedText: qsTr("No GNSS receiver connected.")

    readonly property bool _connected: root.facts.telemetryAvailable
    readonly property gpsReceiverPresentation _presentation: root.receiver.activePresentation
    readonly property bool _surveyConnected: root.receiver.activeBaseMode === BaseModeDefinition.BaseSurveyIn
    // A finished survey is the moment to keep its position, so the next connection can start as a fixed base.
    readonly property bool _surveyComplete: root._connected && root._surveyConnected && !root.facts.active.value
                                            && root.facts.canSaveCurrentBasePosition
    readonly property bool _surveySaved: root.facts.currentBasePositionSaved

    heading: qsTr("GNSS Receiver Status")
    visible: root._connected || root.receiver.hasReceiver || root.receiver.reconnecting || root.showWhenDisconnected

    /// A status Fact, labelled with its description.
    component FactRow: LabelledLabel {
        required property Fact fact
        label: fact.shortDescription
        labelText: fact.enumOrValueString
    }

    /// A label and a value reported from outside, such as a long endpoint, which wraps instead of widening the panel.
    component WrappingRow: RowLayout {
        property alias label: rowLabel.text
        property alias labelText: rowValue.text

        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCLabel { id: rowLabel }
        GPSNoteLabel {
            id: rowValue
            horizontalAlignment: Text.AlignRight
            wrapMode: Text.WrapAnywhere
        }
    }

    GPSNoteLabel {
        objectName: "rtkReceiverStatus"
        text: root.receiver.statusText || root.disconnectedText
    }

    LabelledLabel {
        objectName: "rtkDetectedReceiver"
        visible: root.receiver.hasReceiver && root.receiver.detectedReceiver.length > 0
        label: qsTr("Detected")
        labelText: root.receiver.detectedReceiver
        labelTextFormat: Text.PlainText
    }

    WrappingRow {
        objectName: "rtkReceiverIdentity"
        visible: root._connected && root.receiver.receiverIdentity.length > 0
        label: qsTr("Receiver")
        labelText: root.receiver.receiverIdentity
    }

    WrappingRow {
        objectName: "rtkReceiverEndpoint"
        visible: root.receiver.hasReceiver && root.receiver.activeEndpoint.length > 0
        label: qsTr("Connection")
        labelText: root.receiver.activeEndpoint
    }

    // Values the receiver does not report are not shown. Each row appears with its value, at the Facts' update rate.
    FactRow {
        objectName: "rtkFixType"
        fact: root.facts.fixType
        visible: root._connected && root.facts.fixType.value !== GPSFixQuality.Unknown
    }

    FactRow {
        objectName: "rtkSatellitesInView"
        fact: root.facts.numSatellites
        visible: root._connected && root.facts.numSatellites.value >= 0
    }

    FactRow {
        objectName: "rtkSatellitesUsed"
        fact: root.facts.numSatellitesUsed
        visible: root._connected && root.facts.numSatellitesUsed.value >= 0
    }

    FactRow {
        objectName: "rtkJamming"
        fact: root.facts.jammingState
        visible: root._connected && root.facts.jammingState.rawValue > 0
    }

    FactRow {
        objectName: "rtkSpoofing"
        fact: root.facts.spoofingState
        visible: root._connected && root.facts.spoofingState.rawValue > 0
    }

    FactRow {
        objectName: "rtkAntenna"
        fact: root.facts.antennaState
        visible: root._connected && root.facts.antennaState.rawValue > 0
    }

    LabelledFactLabel {
        objectName: "rtkSurveyDuration"
        label: root._presentation.acceptedObservationTime ? qsTr("Accepted observation time") : qsTr("Duration")
        fact: root.facts.currentDuration
        visible: root._connected && root._presentation.reportsSurveyDuration && root._surveyConnected
    }

    LabelledFactLabel {
        objectName: "rtkSurveyAccuracy"
        label: root.facts.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
        fact: root.facts.currentAccuracy
        visible: root._connected && root._surveyConnected && root.facts.currentAccuracy.value > 0
    }

    GPSNoteLabel {
        objectName: "rtkSurveyCompletePrompt"
        visible: root._surveyComplete
        text: root._surveySaved
              ? qsTr("Survey position saved. Select Fixed position to start from it next time.")
              : qsTr("Survey complete. Save this position to start the base from it next time.")
    }

    QGCButton {
        objectName: "rtkSurveySaveButton"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        wrapMode: Text.Wrap
        visible: root._surveyComplete && !root._surveySaved
        text: qsTr("Save Survey Position")
        onClicked: QGroundControl.gpsManager.saveCurrentBasePosition()
    }
}
