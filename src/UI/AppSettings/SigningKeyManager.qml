pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsGroupLayout {
    id: _signingKeyManager

    Layout.fillWidth: true
    heading:            qsTr("MAVLink 2 Signing")
    headingDescription: qsTr("Signing keys should only be sent to the vehicle over secure links (e.g. USB).")

    property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    // Wipe clipboard 30s after Export so a casually-copied key doesn't linger across paste targets.
    Timer {
        id: clipboardWipeTimer
        interval: 30000
        repeat: false
        onTriggered: QGroundControl.copyToClipboard("")
    }

    QGCPopupDialogFactory {
        id: addKeyDialogFactory
        dialogComponent: addKeyDialogComponent
    }

    Component {
        id: addKeyDialogComponent

        QGCPopupDialog {
            id:                     addKeyDialog
            title:                  qsTr("Add Signing Key")
            buttons:                Dialog.Ok | Dialog.Cancel
            acceptButtonEnabled:    keyNameField.text !== "" &&
                                    (addKeyDialog.useRawKey
                                        ? rawKeyField.text.length === 64
                                        : passphraseField.text.length >= addKeyDialog._minPassphraseLength)

            property bool useRawKey: false
            readonly property int _minPassphraseLength: 8

            onAccepted: {
                let ok = false
                if (addKeyDialog.useRawKey) {
                    ok = QGroundControl.mavlinkSigningKeys.addRawKey(keyNameField.text, rawKeyField.text)
                } else {
                    ok = QGroundControl.mavlinkSigningKeys.addKey(keyNameField.text, passphraseField.text)
                }
                if (!ok) {
                    addKeyDialog.preventClose = true
                    errorLabel.text = qsTr("Could not add key. Name may already exist or input is invalid.")
                }
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

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCRadioButton {
                        text:       qsTr("Passphrase")
                        checked:    !addKeyDialog.useRawKey
                        onClicked:  addKeyDialog.useRawKey = false
                    }
                    QGCRadioButton {
                        text:       qsTr("Raw Key (hex)")
                        checked:    addKeyDialog.useRawKey
                        onClicked:  addKeyDialog.useRawKey = true
                    }
                }

                QGCTextField {
                    id:                     passphraseField
                    visible:                !addKeyDialog.useRawKey
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                    placeholderText:        qsTr("Enter passphrase (min %1 chars)").arg(addKeyDialog._minPassphraseLength)
                    echoMode:               TextInput.Password
                    inputMethodHints:       Qt.ImhNoPredictiveText
                }

                QGCLabel {
                    visible:    !addKeyDialog.useRawKey && passphraseField.text.length > 0 && passphraseField.text.length < addKeyDialog._minPassphraseLength
                    text:       qsTr("Passphrase too short (%1/%2)").arg(passphraseField.text.length).arg(addKeyDialog._minPassphraseLength)
                    color:      QGroundControl.globalPalette.warningText
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                RowLayout {
                    visible:    addKeyDialog.useRawKey
                    spacing:    ScreenTools.defaultFontPixelWidth / 2

                    QGCTextField {
                        id:                     rawKeyField
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 30
                        placeholderText:        qsTr("64 hex characters")
                        maximumLength:          64
                        inputMethodHints:       Qt.ImhNoPredictiveText
                        validator:              RegularExpressionValidator { regularExpression: /[0-9a-fA-F]*/ }
                    }

                    QGCButton {
                        text:       qsTr("Generate")
                        onClicked:  rawKeyField.text = QGroundControl.mavlinkSigningKeys.generateRandomHexKey()
                    }
                }

                QGCLabel {
                    visible:    addKeyDialog.useRawKey && rawKeyField.text.length > 0 && rawKeyField.text.length !== 64
                    text:       qsTr("%1/64 hex characters").arg(rawKeyField.text.length)
                    color:      QGroundControl.globalPalette.warningText
                    font.pointSize: ScreenTools.smallFontPointSize
                }

                QGCLabel {
                    id:             errorLabel
                    visible:        text !== ""
                    color:          QGroundControl.globalPalette.warningText
                    font.pointSize: ScreenTools.smallFontPointSize
                    Layout.fillWidth: true
                    wrapMode:       Text.WordWrap
                }
            }
        }
    }

    LabelledLabel {
        Layout.fillWidth:   true
        label:              qsTr("Active Key")
        labelText:          _signingKeyManager._activeVehicle && _signingKeyManager._activeVehicle.signingController.signingStatus.keyName !== ""
                                ? _signingKeyManager._activeVehicle.signingController.signingStatus.keyName
                                : qsTr("None")
        visible:            _signingKeyManager._activeVehicle
    }

    Repeater {
        model: QGroundControl.mavlinkSigningKeys.keys

        RowLayout {
            id: keyDelegate
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            required property var object
            required property int index

            property string _keyName:           keyDelegate.object.name
            property bool _keyIsActive:         { void(QGroundControl.mavlinkSigningKeys.keyUsageRevision); return QGroundControl.mavlinkSigningKeys.isKeyInUse(keyDelegate._keyName) }
            property bool _keyIsActiveVehicle:  _signingKeyManager._activeVehicle && _signingKeyManager._activeVehicle.signingController.signingStatus.keyName === keyDelegate._keyName
            property bool _anyKeyActive:        _signingKeyManager._activeVehicle && _signingKeyManager._activeVehicle.signingController.signingStatus.keyName !== ""
            property bool _otherKeyActive:      keyDelegate._anyKeyActive && !keyDelegate._keyIsActiveVehicle
            property bool _signingPending:      _signingKeyManager._activeVehicle && _signingKeyManager._activeVehicle.signingController.signingStatus.pending

            QGCLabel {
                text:               keyDelegate._keyName
                Layout.fillWidth:   true
            }

            // Quiet hint when a different key on this vehicle is the active one — explains why
            // Enable/Disable are both hidden on this row.
            QGCLabel {
                text:               qsTr("(another key active)")
                visible:            keyDelegate._otherKeyActive
                font.pointSize:     ScreenTools.smallFontPointSize
                opacity:            0.7
            }

            QGCButton {
                text:       keyDelegate._signingPending ? qsTr("Configuring…") : qsTr("Enable")
                visible:    !keyDelegate._anyKeyActive
                enabled:    _signingKeyManager._activeVehicle && !keyDelegate._signingPending
                onClicked: {
                    if (!_signingKeyManager._activeVehicle) {
                        return
                    }
                    const linkName = _signingKeyManager._activeVehicle.vehicleLinkManager
                                        ? _signingKeyManager._activeVehicle.vehicleLinkManager.primaryLinkName
                                        : qsTr("active link")
                    QGroundControl.showMessageDialog(
                        _signingKeyManager,
                        qsTr("Send Signing Key"),
                        qsTr("This will transmit key '%1' to the vehicle over '%2'. Only proceed if this link is secure (USB or trusted local network).").arg(keyDelegate._keyName).arg(linkName),
                        Dialog.Ok | Dialog.Cancel,
                        function () {
                            if (_signingKeyManager._activeVehicle) {
                                _signingKeyManager._activeVehicle.signingController.enable(keyDelegate._keyName)
                            }
                        })
                }
            }

            QGCButton {
                text:       keyDelegate._signingPending ? qsTr("Disabling…") : qsTr("Disable")
                visible:    keyDelegate._keyIsActiveVehicle
                enabled:    _signingKeyManager._activeVehicle && !keyDelegate._signingPending
                onClicked:  if (_signingKeyManager._activeVehicle) _signingKeyManager._activeVehicle.signingController.disable()
            }

            QGCButton {
                text:       qsTr("Export")
                visible:    !keyDelegate._keyIsActive
                onClicked: {
                    let hex = QGroundControl.mavlinkSigningKeys.keyHexByName(keyDelegate._keyName)
                    if (hex !== "") {
                        QGroundControl.copyToClipboard(hex)
                        clipboardWipeTimer.restart()
                        QGroundControl.showMessageDialog(
                            _signingKeyManager,
                            qsTr("Export Key: %1").arg(keyDelegate._keyName),
                            qsTr("Key copied to clipboard. Store it securely — it will be cleared from the clipboard in 30 seconds."),
                            Dialog.Ok)
                    }
                }
            }

            QGCButton {
                text:       qsTr("Delete")
                visible:    !keyDelegate._keyIsActive
                onClicked:  QGroundControl.showMessageDialog(
                                _signingKeyManager,
                                qsTr("Delete Signing Key"),
                                qsTr("Are you sure you want to delete '%1'?\n\nIf a vehicle still has this key configured, you will no longer be able to communicate with it over a signed connection. Raw or generated keys cannot be recovered — Export the hex first if you may need it later.").arg(keyDelegate._keyName),
                                Dialog.Ok | Dialog.Cancel,
                                function () { QGroundControl.mavlinkSigningKeys.removeKey(keyDelegate._keyName) })
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
