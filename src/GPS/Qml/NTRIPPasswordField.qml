pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

RowLayout {
    id: root

    property bool _showPassword: false
    readonly property SettingsFact _password: QGroundControl.settingsManager.ntripSettings.ntripPassword as SettingsFact

    Layout.fillWidth: true
    visible: root._password.userVisible
    spacing: ScreenTools.defaultFontPixelWidth * 0.5

    LabelledFactTextField {
        Layout.fillWidth: true
        textFieldPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
        fact: root._password
        textField.echoMode: root._showPassword ? TextInput.Normal : TextInput.Password
    }

    QGCButton {
        Layout.alignment: Qt.AlignBottom
        text: root._showPassword ? qsTr("Hide") : qsTr("Show")
        onClicked: root._showPassword = !root._showPassword
    }
}
