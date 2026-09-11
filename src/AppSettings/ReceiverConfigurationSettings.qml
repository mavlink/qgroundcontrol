import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    readonly property var _descriptors: root.settings && root.descriptors ? root.descriptors.filter(descriptor => {
        const fact = root.settings[descriptor.key];
        return fact && fact.userVisible && (descriptor.support !== 1 || Number(fact.rawValue) !== Number(descriptor.defaultValue));
    }) : []
    property var descriptors: QGroundControl.gpsManager.receiverSettings
    property bool receiverActive: QGroundControl.gpsManager.receiverConnection.active
    property var settings: QGroundControl.settingsManager.rtkSettings

    heading: qsTr("Receiver Configuration")
    headingDescription: root.receiverActive ? qsTr("Disconnect the receiver to edit these settings. Changes apply on the next connection.") : qsTr("Changes apply on the next connection.")
    visible: root.settings && root.settings.userVisible && root._descriptors.length > 0

    Repeater {
        model: root._descriptors

        ColumnLayout {
            id: setting

            readonly property bool _editable: setting._fact && !root.receiverActive && setting.modelData.support !== 1
            readonly property Fact _fact: root.settings ? root.settings[setting.modelData.key] : null
            required property var modelData

            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelHeight / 2
            visible: setting._fact && setting._fact.userVisible

            Loader {
                Layout.fillWidth: true
                active: setting.visible && setting.modelData.support !== 1
                sourceComponent: setting.modelData.kind === "flags" ? flagsControl : setting.modelData.kind === "enum" ? enumControl : numberControl
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                text: qsTr("Support for %1 will be checked when the receiver connects.").arg(setting.modelData.label)
                visible: setting.modelData.support === 0
                wrapMode: Text.WordWrap
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                text: qsTr("%1 is unavailable for the selected receiver or role. Reset its saved value to the default before connecting.").arg(setting.modelData.label)
                visible: setting.modelData.support === 1
                wrapMode: Text.WordWrap
            }

            QGCButton {
                enabled: setting._fact && !root.receiverActive
                objectName: "receiverSetting_" + setting.modelData.key + "_reset"
                text: qsTr("Reset %1 to default").arg(setting.modelData.label)
                visible: setting.modelData.support === 1

                onClicked: {
                    if (setting._fact && !root.receiverActive) {
                        setting._fact.rawValue = setting.modelData.defaultValue;
                    }
                }
            }

            Component {
                id: enumControl

                LabelledComboBox {
                    currentIndex: setting._fact ? setting.modelData.values.indexOf(Number(setting._fact.rawValue)) : -1
                    enabled: setting._editable
                    label: setting.modelData.label
                    model: setting.modelData.labels
                    objectName: "receiverSetting_" + setting.modelData.key

                    onActivated: index => {
                        if (setting._editable && index >= 0 && index < setting.modelData.values.length) {
                            setting._fact.rawValue = setting.modelData.values[index];
                        }
                    }
                }
            }

            Component {
                id: flagsControl

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel {
                        Layout.fillWidth: true
                        text: setting.modelData.label
                    }

                    Repeater {
                        model: setting.modelData.values

                        QGCCheckBox {
                            id: flag

                            readonly property int _mask: setting._fact ? Number(setting._fact.rawValue) : 0
                            readonly property bool _required: (flag._requiredMask & flag._value) !== 0
                            readonly property int _requiredMask: Number(setting.modelData.requiredMask || 0)
                            readonly property int _value: Number(flag.modelData)
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            checked: flag._value === 0 ? flag._mask === 0 : (flag._mask & flag._value) === flag._value
                            enabled: setting._editable && (flag._value === 0 || !flag._required || flag._mask === 0)
                            objectName: "receiverSetting_" + setting.modelData.key + "_" + flag.modelData
                            text: setting.modelData.labels[flag.index]

                            onClicked: {
                                if (setting._editable) {
                                    const selected = flag._value === 0 ? 0 : checked ? flag._mask | flag._value | flag._requiredMask : flag._mask & ~flag._value;
                                    setting._fact.rawValue = selected;
                                }
                            }
                        }
                    }
                }
            }

            Component {
                id: numberControl

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        Layout.fillWidth: true
                        text: setting.modelData.label
                        wrapMode: Text.WordWrap
                    }

                    FactTextField {
                        function _onEditingFinished() {
                            if (!setting._editable) {
                                return;
                            }
                            let value = NaN;
                            try {
                                value = Number.fromLocaleString(Qt.locale(), text);
                            } catch (error) {
                                value = NaN;
                            }
                            if (!Number.isFinite(value) || value < setting.modelData.minimum || value > setting.modelData.maximum) {
                                showValidationError(qsTr("Enter a value from %1 to %2.").arg(setting.modelData.minimum).arg(setting.modelData.maximum), fact.valueString);
                                return;
                            }
                            const error = fact.validate(value.toString(), false);
                            if (error !== "") {
                                showValidationError(error, fact.valueString);
                                return;
                            }
                            clearValidationError();
                            fact.value = value;
                            updated();
                        }

                        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 14
                        enabled: setting._editable
                        fact: setting._fact
                        objectName: "receiverSetting_" + setting.modelData.key
                    }
                }
            }
        }
    }
}
