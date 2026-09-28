pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Live GNSS receiver status, shared by the GPS indicator and the receiver settings page.
SettingsGroupLayout {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// The receiver's status Facts.
    property GPSReceiverFactGroup facts: QGroundControl.gpsManager.receiverFacts
    /// Keep the group visible without a receiver, showing disconnectedText.
    property bool showWhenDisconnected: false
    property string disconnectedText: qsTr("No GNSS receiver connected.")

    readonly property bool _connected: root.facts.connected.value
    readonly property gpsReceiverPresentation _presentation: root.receiver.activePresentation
    readonly property bool _surveyConnected: root.receiver.activeBaseMode === BaseModeDefinition.BaseSurveyIn
    readonly property string _na: qsTr("N/A", "No data to display")
    // A finished survey is the moment to keep its position, so the next connection can start as a fixed base.
    readonly property bool _surveyComplete: root._connected && root._surveyConnected && !root.facts.active.value
                                            && root.facts.canSaveCurrentBasePosition
    property bool _surveySaved: false

    on_SurveyCompleteChanged: {
        if (!root._surveyComplete) {
            root._surveySaved = false
        }
    }

    heading: qsTr("GNSS Receiver Status")
    visible: root._connected || root.receiver.hasReceiver || root.receiver.reconnecting || root.showWhenDisconnected

    QGCLabel {
        objectName: "rtkReceiverStatus"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        wrapMode: Text.Wrap
        text: !root._connected
              ? (root.receiver.hasReceiver
                 ? (root._presentation.automatic ? qsTr("Identifying receiver...") : qsTr("Connecting to receiver..."))
                 : root.receiver.reconnecting ? qsTr("Receiver connection lost. Reconnecting...")
                 : root.disconnectedText)
              : root.receiver.activeRole === GPSReceiver.Passive
                ? (root.receiver.forwardingCorrections ? qsTr("Passive receiver connected; forwarding its RTCM")
                                                       : qsTr("Passive receiver connected; position only"))
              : root.receiver.activeBaseMode === BaseModeDefinition.BaseReceiverAveraging
                ? qsTr("Receiver-managed averaging — no accuracy guarantee")
              : root.receiver.activeBaseMode === BaseModeDefinition.BaseFixed ? qsTr("Fixed base position")
              : root.facts.active.value ? qsTr("Survey-in Active") : qsTr("Receiver connected")
    }

    LabelledLabel {
        objectName: "rtkDetectedReceiver"
        Layout.fillWidth: true
        visible: root.receiver.hasReceiver && root.receiver.detectedReceiver.length > 0
        label: qsTr("Detected")
        labelText: root.receiver.detectedReceiver
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        objectName: "rtkReceiverIdentity"
        Layout.fillWidth: true
        visible: root._connected && root.receiver.receiverIdentity.length > 0
        label: qsTr("Receiver")
        labelText: root.receiver.receiverIdentity
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        objectName: "rtkReceiverEndpoint"
        Layout.fillWidth: true
        visible: root.receiver.hasReceiver && root.receiver.activeEndpoint.length > 0
        label: qsTr("Connection")
        labelText: root.receiver.activeEndpoint
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        objectName: "rtkFixType"
        Layout.fillWidth: true
        visible: root._connected
        label: qsTr("Receiver Fix")
        labelText: root.facts.fixType.rawValue === 0 ? root._na : root.facts.fixType.enumStringValue
    }

    LabelledLabel {
        objectName: "rtkSatellitesInView"
        Layout.fillWidth: true
        visible: root._connected
        label: qsTr("Satellites in View")
        labelText: root.facts.numSatellites.rawValue < 0 ? root._na : root.facts.numSatellites.valueString
    }

    LabelledLabel {
        objectName: "rtkSatellitesUsed"
        Layout.fillWidth: true
        visible: root._connected
        label: qsTr("Satellites Used")
        labelText: root.facts.numSatellitesUsed.rawValue < 0 ? root._na : root.facts.numSatellitesUsed.valueString
    }

    LabelledLabel {
        objectName: "rtkJamming"
        Layout.fillWidth: true
        visible: root._connected && root.facts.jammingState.rawValue > 0
        label: qsTr("Jamming")
        labelText: root.facts.jammingState.enumStringValue
    }

    LabelledLabel {
        objectName: "rtkSpoofing"
        Layout.fillWidth: true
        visible: root._connected && root.facts.spoofingState.rawValue > 0
        label: qsTr("Spoofing")
        labelText: root.facts.spoofingState.enumStringValue
    }

    LabelledLabel {
        objectName: "rtkSurveyDuration"
        Layout.fillWidth: true
        label: root._presentation.acceptedObservationTime ? qsTr("Accepted observation time") : qsTr("Duration")
        visible: root._connected && root._presentation.reportsSurveyDuration && root._surveyConnected
        //: %1 is Survey-In duration in seconds
        labelText: qsTr("%1 s").arg(root.facts.currentDuration.value)
    }

    LabelledLabel {
        objectName: "rtkSurveyAccuracy"
        Layout.fillWidth: true
        label: root.facts.valid.value ? qsTr("Accuracy") : qsTr("Current Accuracy")
        labelText: root.facts.currentAccuracy.valueString + " " + root.facts.currentAccuracy.units
        visible: root._connected && root._surveyConnected && root.facts.currentAccuracy.value > 0
    }

    QGCLabel {
        objectName: "rtkSurveyCompletePrompt"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 0
        wrapMode: Text.Wrap
        visible: root._surveyComplete
        text: root._surveySaved
              ? qsTr("Survey position saved. Select Fixed base position to start from it next time.")
              : qsTr("Survey complete. Save this position to start the base from it next time.")
    }

    QGCButton {
        objectName: "rtkSurveySaveButton"
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        wrapMode: Text.Wrap
        visible: root._surveyComplete && !root._surveySaved
        text: qsTr("Save Survey Position")
        onClicked: root._surveySaved = QGroundControl.gpsManager.saveCurrentBasePosition()
    }
}
