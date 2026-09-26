import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

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
    readonly property int _streamCount: root.corrections.streamCount(root.corrections.sourceInstances, root._source)
    readonly property var _streams: root.corrections.streamChoices(root.corrections.sourceInstances, root._source,
                                                                   root._selectedInstance)

    heading: qsTr("Correction Routing")
    headingDescription: qsTr("Selects the stream sent to vehicles. UDP forwarding sends the same stream.")
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
            return qsTr("Uses only the chosen source category, without fallback to other categories. When the category has several streams, one can be pinned; a pinned stream waits if unavailable.");
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
                 && (root._streamCount > 1 || root._selectedInstance !== "")

        onActivated: index => {
            if (index >= 0 && index < root._streams.length) {
                root._settings.correctionSourceInstance.rawValue = streamCombo.currentValue;
            }
        }
    }
}
