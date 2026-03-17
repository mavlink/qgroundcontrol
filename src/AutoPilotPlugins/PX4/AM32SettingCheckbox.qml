import QtQuick
import QGroundControl.Controls

QGCCheckBox {
    required property var firstEeprom
    required property var updateSetting

    property string label: ""
    property string settingName: ""
    property var setting: firstEeprom ? firstEeprom.settings[settingName] : null

    visible: setting && firstEeprom && firstEeprom.isSettingAvailable(settingName)
    text: label + (setting && setting.hasPendingChanges ? " *" : "")
    checked: setting ? setting.fact.rawValue != 0 : false
    textColor: setting && setting.hasPendingChanges ? qgcPal.colorOrange : (setting && !setting.allMatch ? qgcPal.colorRed : qgcPal.text)
    onClicked: updateSetting(settingName, checked)
}
