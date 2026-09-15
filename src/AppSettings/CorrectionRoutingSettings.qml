import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections

    readonly property bool _manual: root._source === GPSCorrectionSettings.LocalReceiver
                                   || root._source === GPSCorrectionSettings.Ntrip
                                   || root._source === GPSCorrectionSettings.Udp
    readonly property string _selectedInstance: root._settings.correctionSourceInstance.rawValue
    readonly property GPSCorrectionSettings _settings: QGroundControl.settingsManager.gpsCorrectionSettings
    readonly property SettingsFact _sourceFact: root._settings.correctionSource as SettingsFact
    readonly property SettingsFact _instanceFact: root._settings.correctionSourceInstance as SettingsFact
    readonly property int _source: root._settings.correctionSource.rawValue
    readonly property var _streams: {
        const streams = [{ instanceId: "", label: qsTr("Automatic within source") }];
        for (const source of root.corrections.sourceInstances) {
            if (source.source === root._source && !streams.some(stream => stream.instanceId === source.instanceId)) {
                streams.push({
                    instanceId: source.instanceId,
                    label: source.usable ? source.instanceId : qsTr("No fresh corrections: %1").arg(source.instanceId)
                });
            }
        }
        if (!streams.some(stream => stream.instanceId === root._selectedInstance)) {
            streams.push({
                instanceId: root._selectedInstance,
                label: qsTr("Unavailable: %1").arg(root._selectedInstance)
            });
        }
        return streams;
    }

    heading: qsTr("Correction Routing")
    headingDescription: qsTr("Selects streams for vehicle links only. NTRIP UDP forwarding uses the NTRIP stream independently.")
    objectName: "correctionRoutingSettings"
    visible: root._settings.userVisible && root._sourceFact && root._sourceFact.userVisible

    LabelledFactComboBox {
        fact: root._sourceFact
        indexModel: false
        label: fact.label
        objectName: "correctionSource"

        onActivated: {
            if (root._instanceFact && root._instanceFact.userVisible) {
                root._settings.correctionSourceInstance.rawValue = "";
            }
        }
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "correctionRoutingDescription"
        text: {
            if (root._source === GPSCorrectionSettings.Automatic)
                return qsTr("Uses one fresh stream, preferring the local base station, then NTRIP, then UDP.");
            if (root._source === GPSCorrectionSettings.All)
                return qsTr("Forwards all fresh streams to vehicles. Corrections from different base stations may be mixed.");
            return qsTr("Uses only the chosen source category, without fallback to other categories. Automatic within source chooses a fresh stream in that category; a pinned stream waits if unavailable.");
        }
        wrapMode: Text.WordWrap
    }

    LabelledComboBox {
        id: streamCombo

        comboBoxPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
        currentValue: root._selectedInstance
        label: qsTr("Stream")
        model: root._streams
        objectName: "correctionStream"
        textRole: "label"
        valueRole: "instanceId"
        visible: root._manual && root._instanceFact && root._instanceFact.userVisible

        onActivated: index => {
            if (index >= 0 && index < root._streams.length) {
                root._settings.correctionSourceInstance.rawValue = streamCombo.currentValue;
            }
        }
    }
}
