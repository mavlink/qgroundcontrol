import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id: root

    Layout.fillWidth: true
    heading: qsTr("Network Video Security")

    property var _settings: QGroundControl.settingsManager.videoSettings
    property bool _usesAuthentication: _settings.networkVideoAuthType.rawValue !== VideoSettings.NetworkVideoAuthNone
    property bool _usesBasicAuthentication: (
        _settings.networkVideoAuthType.rawValue === VideoSettings.NetworkVideoAuthBasic
    )
    property string _credentialMessage: ""
    property string _configurationError: _settings.networkVideoConfigurationError

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: root.enabled
    }

    LabelledFactComboBox {
        Layout.fillWidth: true
        fact: root._settings.networkVideoAuthType
        indexModel: false
    }

    LabelledFactTextField {
        Layout.fillWidth: true
        textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 40
        fact: root._settings.networkVideoUsername
        visible: root._usesBasicAuthentication
    }

    ColumnLayout {
        Layout.fillWidth: true
        visible: root._usesAuthentication
        spacing: ScreenTools.defaultFontPixelHeight / 2

        QGCLabel {
            Layout.fillWidth: true
            text: root._usesBasicAuthentication ? qsTr("Session password") : qsTr("Session bearer token")
        }

        QGCTextField {
            id: sessionSecret
            Layout.fillWidth: true
            echoMode: TextInput.Password
            maximumLength: 4096
            placeholderText: root._settings.networkVideoSessionSecretConfigured
                ? qsTr("Credential configured")
                : qsTr("Enter credential")
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                text: qsTr("Use")
                enabled: sessionSecret.text.length > 0
                onClicked: {
                    root._credentialMessage = root._settings.setNetworkVideoSecret(sessionSecret.text)
                    if (root._credentialMessage.length === 0) {
                        sessionSecret.clear()
                    }
                }
            }

            QGCButton {
                text: qsTr("Clear")
                enabled: root._settings.networkVideoSessionSecretConfigured
                onClicked: {
                    root._settings.clearNetworkVideoSecret()
                    sessionSecret.clear()
                    root._credentialMessage = ""
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }

    LabelledFactBrowse {
        Layout.fillWidth: true
        fact: root._settings.networkVideoSecretFile
        selectFolder: false
        visible: root._usesAuthentication && root._settings.networkVideoCredentialFileSupported
    }

    LabelledFactTextField {
        Layout.fillWidth: true
        textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 40
        fact: root._settings.networkVideoOrigin
    }

    LabelledFactBrowse {
        Layout.fillWidth: true
        fact: root._settings.networkVideoCaCertificateFile
        selectFolder: false
        visible: !ScreenTools.isMobile
    }

    QGCLabel {
        Layout.fillWidth: true
        color: qgcPal.warningText
        text: root._credentialMessage.length > 0 ? root._credentialMessage : root._configurationError
        visible: text.length > 0
        wrapMode: Text.WordWrap
    }
}
