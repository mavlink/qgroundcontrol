import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    id: _signingKeyManager

    Layout.fillWidth: true
    heading:            qsTr("MAVLink 2 Signing")
    headingDescription: qsTr("Signing keys should only be sent to the vehicle over secure links.")

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    // Reactive key-usage tracking (re-evaluated on any vehicle's signing change)
    property int _keyUsageRevision: 0
    Connections {
        target: QGroundControl.mavlinkSigningKeys
        function onKeyUsageChanged() { _keyUsageRevision++ }
    }

    QGCPopupDialogFactory {
        id: addKeyDialogFactory
        dialogComponent: addKeyDialogComponent
    }

    Component {
        id: addKeyDialogComponent

        QGCPopupDialog {
            title:                  qsTr("Add Signing Key")
            buttons:                Dialog.Ok | Dialog.Cancel
            acceptButtonEnabled:    keyNameField.text !== "" && keyPassphraseField.text !== ""

            onAccepted: {
                QGroundControl.mavlinkSigningKeys.addKey(keyNameField.text, keyPassphraseField.text)
            }

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight / 2

                QGCLabel { text: qsTr("Key Name") }
                QGCTextField {
                    id:                     keyNameField
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                    placeholderText:        qsTr("Enter a friendly name")
                }

                QGCLabel { text: qsTr("Passphrase") }
                QGCTextField {
                    id:                     keyPassphraseField
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                    placeholderText:        qsTr("Enter passphrase")
                    echoMode:               TextInput.Password
                }
            }
        }
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Signing Status")
        labelText:          _activeVehicle ? (_activeVehicle.mavlinkSigning ? qsTr("Active") : qsTr("Inactive")) : qsTr("Not Connected")
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Active Key")
        labelText:          _activeVehicle && _activeVehicle.mavlinkSigningKeyName !== "" ? _activeVehicle.mavlinkSigningKeyName : qsTr("None")
        visible:            _activeVehicle
    }

    Repeater {
        model: QGroundControl.mavlinkSigningKeys.keys

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            property bool _keyIsActive:         { void(_keyUsageRevision); return QGroundControl.mavlinkSigningKeys.isKeyInUse(object.name) }
            property bool _keyIsActiveVehicle:  _activeVehicle && _activeVehicle.mavlinkSigningKeyName === object.name
            property bool _anyKeyActive:        _activeVehicle && _activeVehicle.mavlinkSigningKeyName !== ""

            QGCLabel {
                text:               object.name
                Layout.fillWidth:   true
            }

            QGCButton {
                text:       qsTr("Enable")
                visible:    !parent._anyKeyActive
                enabled:    _activeVehicle
                onClicked:  _activeVehicle.sendSetupSigning(index)
            }

            QGCButton {
                text:       qsTr("Disable")
                visible:    parent._keyIsActiveVehicle
                onClicked:  _activeVehicle.sendDisableSigning()
            }

            QGCButton {
                text:       qsTr("Delete")
                visible:    !parent._keyIsActive
                onClicked:  QGroundControl.showMessageDialog(
                                _signingKeyManager,
                                qsTr("Delete Signing Key"),
                                qsTr("Are you sure you want to delete '%1'? If a vehicle still has this key configured, you will no longer be able to communicate with it over a signed connection.").arg(object.name),
                                Dialog.Ok | Dialog.Cancel,
                                function () { QGroundControl.mavlinkSigningKeys.removeKey(index) })
            }
        }
    }

    QGCLabel {
        text:       qsTr("No keys configured")
        visible:    QGroundControl.mavlinkSigningKeys.keys.count === 0
    }

    QGCButton {
        text:       qsTr("Add Key")
        onClicked:  addKeyDialogFactory.open()
    }
}
