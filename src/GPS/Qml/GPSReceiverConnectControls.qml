pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.GPS

/// How and when the receiver connects: the corrections format and the consent to save to its flash, automatic
/// connection and recording. Connect itself is GPSReceiverConnectionRow's, with the receiver's state and problem.
GPSReceiverSettingsGroup {
    id: root

    heading: qsTr("Connect")

    GPSFactSwitchField {
        fact: root.settings.compactRtcmCorrections
        note: root.settings.compactRtcmCorrections.shortDescription
        shown: root.configuredBase && root.presentation.compactObservations
        toggle.objectName: "rtkCompactRtcm"
        toggle.enabled: root._editable
    }

    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root.presentation.persistentConfiguration

        QGCCheckBox {
            objectName: "rtkPersistentChangesCheckBox"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root.presentation.automatic ? qsTr("Allow flash save and restart if a Quectel receiver is identified")
                                              : qsTr("Allow flash save and restart")
            focusPolicy: Qt.StrongFocus
            wrapMode: Text.Wrap
            enabled: root._editable
            checked: root.receiver.persistentChangesAllowed
            onClicked: root.receiver.persistentChangesAllowed = checked
        }

        QGCNoteLabel {
            objectName: "rtkPersistentConsentWarning"
            text: root._quectelOnly + qsTr("For this connection only, allow QGroundControl to write requested base role or base-setting changes to receiver flash and restart it. Changes may remain saved even if reconnecting fails. No factory reset is performed. Permission is cleared after each attempt and is never used by automatic connections. To use the receiver as a rover again, restore its role with Quectel QGNSS or $PQTMCFGRCVRMODE,W,1 followed by $PQTMSAVEPAR.")
        }
    }

    GPSFactSwitchField {
        fact: root.autoConnectFact
        note: root.autoConnectFact.rawValue ? root.autoConnectFact.shortDescription : ""
        toggle.objectName: "rtkAutoConnect"
    }

    GPSReceiverRecording { receiver: root.receiver }

    // The permission is for the configuration shown here, so leaving the page withdraws it.
    Component.onDestruction: {
        if (receiver) {
            receiver.persistentChangesAllowed = false
        }
    }
}
