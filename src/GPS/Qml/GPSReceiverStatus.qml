pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// Live GNSS receiver status, shared by the GPS indicator and the receiver settings page.
QGCGroupBox {
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
    readonly property list<gpsConstellationSatellites> _constellations: root.receiver.constellations

    /// "GPS 8/10, Galileo 6/7": the satellites each constellation uses of those in view. No-break spaces keep a
    /// constellation and its counts on one line.
    function _constellationsText(constellations) {
        const texts = []
        for (let i = 0; i < constellations.length; i++) {
            const used = constellations[i].used
            //: %1 is a satellite constellation, such as Galileo; %2 the number of its satellites the receiver uses,
            //: or a dash when unknown; %3 the number in view. The space between them is a no-break space.
            texts.push(qsTr("%1\u00A0%2/%3").arg(constellations[i].name).arg(used >= 0 ? used : "–")
                                            .arg(constellations[i].inView))
        }
        return texts.join(", ")
    }

    heading: qsTr("GNSS Receiver Status")
    visible: root._connected || root.receiver.hasReceiver || root.receiver.reconnecting || root.showWhenDisconnected

    /// A status Fact, labelled with its description.
    component FactRow: LabelledLabel {
        required property Fact fact
        label: fact.shortDescription
        labelText: fact.enumOrValueString
        labelTextFormat: Text.PlainText
    }

    /// A label and a value reported from outside, such as a long endpoint, which wraps instead of widening the panel.
    component WrappingRow: RowLayout {
        property alias label: rowLabel.text
        property alias labelText: rowValue.text
        property alias labelWrapMode: rowValue.wrapMode

        spacing: ScreenTools.defaultFontPixelWidth * 2

        QGCLabel {
            id: rowLabel
            textFormat: Text.PlainText
        }
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

    WrappingRow {
        objectName: "rtkConstellations"
        visible: root._connected && root._constellations.length > 0
        //: Label of the satellites used and in view of each satellite constellation, such as GPS and Galileo
        label: qsTr("Constellations")
        labelText: root._constellationsText(root._constellations)
        labelWrapMode: Text.Wrap
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
        visible: surveySave.offered
        text: root._surveySaved
              ? qsTr("Survey position saved. Select Fixed position to start from it next time.")
              : qsTr("Survey complete. Save this position to start the base from it next time. A site name also keeps it as a saved base site.")
    }

    ColumnLayout {
        id: surveySave

        // GPSManager saves the position of the application's receiver, so only a panel that shows it offers the save.
        readonly property bool offered: root._surveyComplete && root.receiver === QGroundControl.gpsManager.receiver
                                        && root.facts === root.receiver.facts
        property bool failed: false

        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: offered && !root._surveySaved
        onVisibleChanged: failed = false

        QGCTextField {
            id: surveySiteName
            objectName: "rtkSurveySiteName"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            maximumLength: RTKSettings.MaxBaseSiteNameLength
            placeholderText: qsTr("Site name (optional)")
        }

        QGCButton {
            objectName: "rtkSurveySaveButton"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            wrapMode: Text.Wrap
            text: qsTr("Save Survey Position")
            onClicked: surveySave.failed = !QGroundControl.gpsManager.saveCurrentBasePosition(surveySiteName.text)
        }

        GPSNoteLabel {
            objectName: "rtkSurveySaveFailed"
            visible: surveySave.failed
            warning: true
            text: qsTr("The survey position was not saved. The list of saved base sites is full: delete a site in the receiver settings, or save without a site name.")
        }

        Connections {
            target: QGroundControl.settingsManager.rtkSettings
            function onBaseSitesChanged() { surveySave.failed = false }
        }
    }
}
