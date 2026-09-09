import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    readonly property var _corrections: QGroundControl.gpsManager.corrections
    readonly property var _instanceLabels: root._instances.map(value => value === "" ? qsTr("Automatic") : value)
    readonly property var _instances: {
        const values = [""];
        const sources = root._corrections.sourceInstances;
        for (const source of sources) {
            if (source.source === root._source && values.indexOf(source.instanceId) < 0) {
                values.push(source.instanceId);
            }
        }
        if (root._selectedInstance !== "" && values.indexOf(root._selectedInstance) < 0) {
            values.push(root._selectedInstance);
        }
        return values;
    }
    readonly property bool _manual: root._source >= 1 && root._source <= 3
    readonly property string _selectedInstance: root._settings.correctionSourceInstance.rawValue
    readonly property var _settings: QGroundControl.settingsManager.ntripSettings
    readonly property int _source: root._settings.correctionSource.rawValue

    heading: qsTr("Correction Routing")
    objectName: "correctionRoutingSettings"
    visible: root._settings.correctionSource.userVisible

    LabelledFactComboBox {
        fact: root._settings.correctionSource
        indexModel: false
        label: qsTr("Correction source")
        objectName: "correctionSource"

        onActivated: root._settings.correctionSourceInstance.rawValue = ""
    }

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("Uses one fresh stream, preferring the local base station, then NTRIP, then UDP.")
        visible: root._source === 0
        wrapMode: Text.WordWrap
    }

    LabelledComboBox {
        currentIndex: root._instances.indexOf(root._selectedInstance)
        label: qsTr("Stream")
        model: root._instanceLabels
        objectName: "correctionStream"
        visible: root._manual

        onActivated: index => {
            if (index >= 0 && index < root._instances.length) {
                root._settings.correctionSourceInstance.rawValue = root._instances[index];
            }
        }
    }

    FactCheckBox {
        fact: root._settings.injectLocalReceiver
        objectName: "injectLocalReceiver"
        text: qsTr("Send corrections to local receiver")
    }

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("The local receiver must be connected in Position mode and support correction input. Connected vehicles continue receiving corrections.")
        visible: root._settings.injectLocalReceiver.rawValue
        wrapMode: Text.WordWrap
    }
}
