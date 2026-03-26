import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("Server")
    visible:            _ntrip.ntripServerHostAddress.visible || _ntrip.ntripServerPort.visible ||
                        _ntrip.ntripUsername.visible || _ntrip.ntripPassword.visible

    property var  _ntrip:       QGroundControl.settingsManager.ntripSettings
    property bool _isActive:    _ntrip.ntripServerConnectEnabled.rawValue
    property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 30

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        fact:               _ntrip.ntripServerHostAddress
        visible:            fact.visible
        enabled:            !_isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        fact:               _ntrip.ntripServerPort
        visible:            fact.visible
        enabled:            !_isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        label:              fact.shortDescription
        fact:               _ntrip.ntripUsername
        visible:            fact.visible
        enabled:            !_isActive
    }

    RowLayout {
        Layout.fillWidth:   true
        visible:            _ntrip.ntripPassword.visible
        spacing:            ScreenTools.defaultFontPixelWidth * 0.5

        LabelledFactTextField {
            id:                 passwordField
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            label:              fact.shortDescription
            fact:               _ntrip.ntripPassword
            textField.echoMode: _showPassword ? TextInput.Normal : TextInput.Password
            enabled:            !_isActive

            property bool _showPassword: false
        }

        QGCButton {
            text:       passwordField._showPassword ? qsTr("Hide") : qsTr("Show")
            onClicked:  passwordField._showPassword = !passwordField._showPassword
            Layout.alignment: Qt.AlignBottom
        }
    }

    FactCheckBoxSlider {
        Layout.fillWidth:   true
        text:               fact.shortDescription
        fact:               _ntrip.ntripUseTls
        visible:            fact.visible
        enabled:            !_isActive
    }
}
