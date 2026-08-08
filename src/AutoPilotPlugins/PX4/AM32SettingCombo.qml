import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

// Enum/combo control for an AM32 setting. Options come from the Fact's enum
// metadata (populated from the schema, version-gated), and selections are
// written through the shared updateSetting() so all selected ESCs are updated.
ColumnLayout {
    id: control

    required property var firstEeprom
    required property var updateSetting

    property string label: ""
    property string settingName: ""
    property var setting: firstEeprom ? firstEeprom.settings[settingName] : null
    property var fact: setting ? setting.fact : null

    visible: setting && firstEeprom && firstEeprom.isSettingAvailable(settingName)
    spacing: ScreenTools.defaultFontPixelHeight / 4

    QGCLabel {
        text: control.label + (control.setting && control.setting.hasPendingChanges ? " *" : "")
        color: control.setting && control.setting.hasPendingChanges
               ? qgcPal.colorOrange
               : (control.setting && !control.setting.allMatch ? qgcPal.colorRed : qgcPal.text)
    }

    QGCComboBox {
        id: combo
        Layout.fillWidth: true
        sizeToContents: true
        model: control.fact ? control.fact.enumStrings : []
        currentIndex: control.fact ? control.fact.enumIndex : -1

        // ComboBox resets currentIndex to 0 when the model changes (e.g. when a
        // version override swaps the enum set), so re-establish the binding.
        onModelChanged: Qt.callLater(function() {
            combo.currentIndex = Qt.binding(function() {
                return control.fact ? control.fact.enumIndex : -1
            })
        })

        onActivated: (index) => {
            if (control.fact && index >= 0 && index < control.fact.enumValues.length) {
                control.updateSetting(control.settingName, control.fact.enumValues[index])
            }
        }
    }
}
