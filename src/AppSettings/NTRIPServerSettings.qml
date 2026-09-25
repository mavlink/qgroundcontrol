import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    id:                 root
    Layout.fillWidth:   true
    heading:            qsTr("NTRIP Server")
    visible:            _ntrip.ntripServerHostAddress.userVisible || _ntrip.ntripServerPort.userVisible ||
                        _ntrip.ntripUsername.userVisible || _ntrip.ntripPassword.userVisible

    // NTRIPSettings is not registered for QML, and SettingsGroup is not either.
    property var  _ntrip:       QGroundControl.settingsManager.ntripSettings
    property bool _isActive:    _ntrip.ntripServerConnectEnabled.rawValue
    property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 30

    LabelledFactTextField {
        objectName:         "ntripHostField"
        Layout.fillWidth:           true
        textFieldPreferredWidth:    root._textFieldWidth
        fact:               root._ntrip.ntripServerHostAddress
        visible:            (fact as SettingsFact).userVisible
        enabled:            !root._isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    root._textFieldWidth
        fact:               root._ntrip.ntripServerPort
        visible:            (fact as SettingsFact).userVisible
        enabled:            !root._isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    root._textFieldWidth
        label:              fact.shortDescription
        fact:               root._ntrip.ntripUsername
        visible:            (fact as SettingsFact).userVisible
        enabled:            !root._isActive
    }

    RowLayout {
        Layout.fillWidth:   true
        visible:            root._ntrip.ntripPassword.userVisible
        spacing:            ScreenTools.defaultFontPixelWidth * 0.5

        LabelledFactTextField {
            id:                 passwordField
            Layout.fillWidth:           true
            textFieldPreferredWidth:    root._textFieldWidth
            label:              fact.shortDescription
            fact:               root._ntrip.ntripPassword
            textField.echoMode: _showPassword ? TextInput.Normal : TextInput.Password
            enabled:            !root._isActive

            property bool _showPassword: false
        }

        QGCButton {
            text:       passwordField._showPassword ? qsTr("Hide") : qsTr("Show")
            onClicked:  passwordField._showPassword = !passwordField._showPassword
            Layout.alignment: Qt.AlignBottom
        }
    }

    FactCheckBoxSlider {
        objectName:         "ntripUseTlsSwitch"
        Layout.fillWidth:   true
        text:               fact.shortDescription
        fact:               root._ntrip.ntripUseTls
        visible:            (fact as SettingsFact).userVisible
        enabled:            !root._isActive
    }

    FactCheckBoxSlider {
        objectName:         "ntripAcceptSelfSignedSwitch"
        Layout.fillWidth:   true
        text:               fact.shortDescription
        fact:               root._ntrip.ntripAllowSelfSignedCerts
        visible:            (fact as SettingsFact).userVisible
        enabled:            !root._isActive && root._ntrip.ntripUseTls.rawValue
    }
}
