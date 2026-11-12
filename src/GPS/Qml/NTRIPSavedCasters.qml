pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The NTRIP casters kept under a name: using or deleting one, and saving the caster that is entered as one.
QGCGroupBox {
    id: root

    readonly property NTRIPSettings _ntrip: QGroundControl.settingsManager.ntripSettings
    /// The caster and mountpoint settings are locked while the connection is on, and a saved caster would write them.
    readonly property bool _isActive: root._ntrip.ntripServerConnectEnabled.rawValue

    Layout.fillWidth: true
    heading: qsTr("Saved Casters")
    visible: root._ntrip.userVisible

    // Saving or deleting a caster leaves the connection as it is, so only using one waits for a disconnect.
    GPSField {
        objectName: "ntripSavedCasterList"
        label: qsTr("Saved casters")
        note: root._isActive
              ? qsTr("Disconnect to use a saved caster.")
              : qsTr("Use Caster replaces the caster and mountpoint settings above with the selected caster.")
        visible: root._ntrip.savedCasters.length > 0

        QGCComboBox {
            id: casterPicker
            objectName: "ntripSavedCasterPicker"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            model: root._ntrip.savedCasters
            textRole: "name"
        }

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                objectName: "ntripSavedCasterUse"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.Wrap
                focusPolicy: Qt.StrongFocus
                text: qsTr("Use Caster")
                enabled: !root._isActive
                onClicked: root._ntrip.useSavedCaster(casterPicker.currentText)
            }

            QGCButton {
                objectName: "ntripSavedCasterDelete"
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                wrapMode: Text.Wrap
                focusPolicy: Qt.StrongFocus
                text: qsTr("Delete Caster")
                onClicked: root._ntrip.removeSavedCaster(casterPicker.currentText)
            }
        }
    }

    GPSField {
        label: qsTr("Save the caster and mountpoint settings above under a name")
        note: qsTr("A saved caster keeps its username and password.")

        QGCTextField {
            id: casterName
            objectName: "ntripSavedCasterName"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            maximumLength: NTRIPSettings.MaxSavedCasterNameLength
            placeholderText: qsTr("Caster name")
        }

        QGCButton {
            id: casterSave
            objectName: "ntripSavedCasterSave"

            property bool failed: false

            Layout.fillWidth: true
            Layout.minimumWidth: 0
            wrapMode: Text.Wrap
            focusPolicy: Qt.StrongFocus
            text: qsTr("Save Caster")
            enabled: casterName.text.trim() !== "" && root._ntrip.ntripServerHostAddress.rawValue !== ""
            onClicked: {
                failed = !root._ntrip.saveConnectionAsCaster(casterName.text)
                if (!failed) {
                    casterPicker.currentIndex = casterPicker.find(casterName.text.trim())
                }
            }
        }

        QGCNoteLabel {
            objectName: "ntripSavedCasterSaveFailed"
            visible: casterSave.failed
            warning: true
            text: qsTr("The caster was not saved. Its host address is not valid, or the list of saved casters is full: delete a caster first.")
        }
    }

    GPSFactSwitchField {
        fact: root._ntrip.ntripFailoverEnabled
        note: root._ntrip.ntripFailoverEnabled.shortDescription
        toggle.objectName: "ntripFailoverSwitch"
    }

    Connections {
        target: root._ntrip
        function onSavedCastersChanged() { casterSave.failed = false }
    }
}
