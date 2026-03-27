import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var _ntripSettings: QGroundControl.settingsManager.ntripSettings

    NtripServerSettings {
        Layout.fillWidth: true
    }

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:            qsTr("NTRIP Options")
        headingDescription: qsTr("GGA position source, message filtering, and UDP forwarding")
        showDividers:       true

        LabelledFactComboBox {
            Layout.fillWidth: true
            label: fact.label
            fact:  _ntripSettings.ntripGgaPositionSource
            indexModel: false
            visible: fact.userVisible
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            label: fact.label
            fact:  _ntripSettings.ntripWhitelist
            visible: fact.userVisible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
            textField.placeholderText: qsTr("e.g. 1005,1077,1087")
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: fact.label
            fact: _ntripSettings.ntripUdpForwardEnabled
            visible: fact.userVisible
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            label: fact.label
            fact:  _ntripSettings.ntripUdpTargetAddress
            visible: fact.userVisible
            enabled: _ntripSettings.ntripUdpForwardEnabled.rawValue
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
        }

        LabelledFactTextField {
            Layout.fillWidth: true
            label: fact.label
            fact:  _ntripSettings.ntripUdpTargetPort
            visible: fact.userVisible
            enabled: _ntripSettings.ntripUdpForwardEnabled.rawValue
        }
    }
}
