import QtQuick

import QGroundControl
import QGroundControl.Controls

// A single bordered slider cell (label + slider + per-ESC match dots) for an
// AM32 numeric setting. extraVisible allows dependent-field gating.
Rectangle {
    id: cell

    required property var firstEeprom
    required property var eeproms
    required property var updateSetting

    property string label: ""
    property string settingName: ""
    property bool extraVisible: true

    readonly property real _groupMargins: ScreenTools.defaultFontPixelHeight / 2

    visible: firstEeprom && firstEeprom.isSettingAvailable(settingName) && extraVisible
    color: "transparent"
    border.color: qgcPal.groupBorder
    border.width: 1
    radius: 4
    implicitWidth: sliderColumn.width + _groupMargins * 2
    implicitHeight: sliderColumn.height + _groupMargins * 2

    AM32EscMatchDots {
        settingName: cell.settingName
        eeproms: cell.eeproms
    }

    AM32SettingSlider {
        id: sliderColumn
        anchors.centerIn: parent
        label: cell.label
        setting: cell.firstEeprom ? cell.firstEeprom.settings[cell.settingName] : null
        onUserValueChanged: function(newValue) { cell.updateSetting(cell.settingName, newValue) }
    }
}
