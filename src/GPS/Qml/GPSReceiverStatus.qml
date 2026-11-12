pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// The GNSS receiver's connection, with Connect and the receiver's problem, and its live status, shared by the GPS
/// indicator and the receiver settings page. The page has the connection below the settings it connects, and shows
/// the status without it.
QGCGroupBox {
    id: root

    property GPSReceiver receiver: QGroundControl.gpsManager.receiver
    /// The receiver's status Facts.
    property GPSReceiverFactGroup facts: root.receiver.facts
    /// Show the connection, with Connect, above the status.
    property bool showConnection: true
    /// Keep the group, and so Connect, visible without a receiver.
    property bool showWhenDisconnected: false
    /// What the connection row says without a receiver.
    property alias disconnectedText: connection.disconnectedText

    readonly property bool _connected: root.receiver.state === GPSReceiver.Connected
    /// A connection is open or being opened, so there is something to report.
    readonly property bool _reports: root._connected || root.receiver.state === GPSReceiver.Connecting
    readonly property bool _surveyConnected: root.receiver.activeBaseMode === BaseModeDefinition.BaseSurveyIn
    // A finished survey is the moment to keep its position, so the next connection can start as a fixed base.
    readonly property bool _surveyComplete: root._connected && root._surveyConnected
                                            && root.facts.baseState.value !== GPSReceiverFactGroup.BaseState.Active
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
    // A receiver that waits to be connected again has the connection's row to say so, and nothing to report.
    visible: root.showConnection ? root.receiver.state !== GPSReceiver.Disconnected || root.showWhenDisconnected
                                 : root._reports

    /// A status Fact, labelled with its description.
    component FactRow: LabelledLabel {
        required property Fact fact
        label: fact.shortDescription
        labelText: fact.enumOrValueString
        labelTextFormat: Text.PlainText
    }

    GPSReceiverConnectionRow {
        id: connection
        receiver: root.receiver
        visible: root.showConnection
    }

    LabelledLabel {
        objectName: "rtkDetectedReceiver"
        visible: root.receiver.detectedReceiver.length > 0
        label: qsTr("Detected")
        labelText: root.receiver.detectedReceiver
        labelTextFormat: Text.PlainText
    }

    GPSWrappingRow {
        objectName: "rtkReceiverIdentity"
        visible: root._connected && root.receiver.receiverIdentity.length > 0
        label: qsTr("Receiver")
        labelText: root.receiver.receiverIdentity
    }

    GPSWrappingRow {
        objectName: "rtkReceiverEndpoint"
        visible: root.receiver.activeEndpoint.length > 0
        label: qsTr("Connection")
        labelText: root.receiver.activeEndpoint
    }

    // Values the receiver does not report are not shown. Each row appears with its value, at the Facts' update rate.
    // The name of an RTK fix is wider than the status column of the GPS indicator's drop-down has room for beside
    // its label, so it wraps.
    GPSWrappingRow {
        objectName: "rtkFixType"
        visible: root._connected && root.facts.fixType.value !== GPSFixQuality.Unknown
        label: root.facts.fixType.shortDescription
        labelText: root.facts.fixType.enumOrValueString
        labelWrapMode: Text.Wrap
    }

    // What a base is doing comes before the satellites it does it with.
    GPSReceiverSurveyProgress { receiver: root.receiver; facts: root.facts }

    GPSReceiverBasePosition { receiver: root.receiver; facts: root.facts }

    GPSReceiverBaseCheck { receiver: root.receiver }

    QGCNoteLabel {
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

        QGCNoteLabel {
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

    GPSWrappingRow {
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

    // Last: the tallest part of the status, and the one needed least often.
    GPSSatelliteView {
        objectName: "rtkSatelliteView"
        satellites: root.receiver.satellites
        visible: root._connected && root.receiver.satellites.count > 0
    }
}
