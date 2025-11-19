/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.NTRIP 1.0

SettingsPage {
    property var _settingsManager:   QGroundControl.settingsManager
    property var _ntrip:             _settingsManager.ntripSettings
    property Fact _enabled:          _ntrip.ntripServerConnectEnabled

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("NTRIP / RTK")
        visible:            _ntrip.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               _enabled.shortDescription
            fact:               _enabled
            visible:            _enabled.visible
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        visible:            _ntrip.ntripServerHostAddress.visible || _ntrip.ntripServerPort.visible ||
                            _ntrip.ntripUsername.visible || _ntrip.ntripPassword.visible ||
                            _ntrip.ntripMountpoint.visible || _ntrip.ntripWhitelist.visible ||
                            _ntrip.ntripUseSpartn.visible
        enabled:            _enabled.rawValue

        // Status line
        QGCLabel {
            Layout.fillWidth:   true
            Layout.minimumHeight: 30
            visible: true
            text: {
                try {
                    return NTRIPManager ? (NTRIPManager.ntripStatus || "Disconnected") : "NTRIP Manager not available"
                } catch (e) {
                    return "Disconnected"
                }
            }
            wrapMode: Text.WordWrap
            color: {
                try {
                    if (!NTRIPManager) return qgcPal.text
                    var status = NTRIPManager.ntripStatus || ""
                    if (status.toLowerCase().includes("connected")) return qgcPal.colorGreen
                    if (status.toLowerCase().includes("connecting")) return qgcPal.colorOrange
                    if (status.toLowerCase().includes("error") || status.toLowerCase().includes("failed")) return qgcPal.colorRed
                    return qgcPal.text
                } catch (e) {
                    return qgcPal.text
                }
            }
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripServerHostAddress.shortDescription
            fact:               _ntrip.ntripServerHostAddress
            visible:            _ntrip.ntripServerHostAddress.visible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 60
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripServerPort.shortDescription
            fact:               _ntrip.ntripServerPort
            visible:            _ntrip.ntripServerPort.visible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 20
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripUsername.shortDescription
            fact:               _ntrip.ntripUsername
            visible:            _ntrip.ntripUsername.visible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 60
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripPassword.shortDescription
            fact:               _ntrip.ntripPassword
            visible:            _ntrip.ntripPassword.visible
            textField.echoMode: TextInput.Password
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 60
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripMountpoint.shortDescription
            fact:               _ntrip.ntripMountpoint
            visible:            _ntrip.ntripMountpoint.visible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 40
        }

        LabelledFactTextField {
            Layout.fillWidth:   true
            label:              _ntrip.ntripWhitelist.shortDescription
            fact:               _ntrip.ntripWhitelist
            visible:            _ntrip.ntripWhitelist.visible
            textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 40
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               _ntrip.ntripUseSpartn.shortDescription
            fact:               _ntrip.ntripUseSpartn
            visible:            _ntrip.ntripUseSpartn.visible
            enabled:            false
        }
    }
}
