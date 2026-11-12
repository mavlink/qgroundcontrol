pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// The one-use permission to save a base configuration to the receiver's flash, with what giving it means, for the
/// receiver family that needs it. While the family is still to be detected, both are behind a row that shows and hides
/// them, so that the users of other receivers need not read them, and they show once a receiver has asked for the
/// permission.
ColumnLayout {
    id: root

    required property GPSReceiver receiver
    property bool editable: true
    /// The row has shown the permission and its explanation. A selected manufacturer has no row: they are shown.
    property bool expanded: false

    readonly property bool _automatic: root.receiver.configuredPresentation.automatic
    readonly property bool _required: root.receiver.persistentChangesRequired

    function _showIfRequired() {
        if (root._required) {
            root.expanded = true
        }
    }

    Layout.minimumWidth: 0
    spacing: ScreenTools.defaultFontPixelHeight / 4
    visible: root.receiver.configuredPresentation.persistentConfiguration

    on_RequiredChanged: root._showIfRequired()

    LabelledButton {
        objectName: "rtkPersistentChangesToggle"
        visible: root._automatic
        label: qsTr("Flash save for Quectel receivers")
        buttonText: root.expanded ? qsTr("Hide") : qsTr("Show")
        onClicked: {
            root.expanded = !root.expanded
            // The permission is given where it can be seen.
            if (!root.expanded) {
                root.receiver.persistentChangesAllowed = false
            }
        }
    }

    ColumnLayout {
        Layout.minimumWidth: 0
        spacing: root.spacing
        visible: !root._automatic || root.expanded

        QGCCheckBox {
            objectName: "rtkPersistentChangesCheckBox"
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: root._automatic ? qsTr("Allow flash save and restart if a Quectel receiver is identified")
                                  : qsTr("Allow flash save and restart")
            focusPolicy: Qt.StrongFocus
            wrapMode: Text.Wrap
            enabled: root.editable
            checked: root.receiver.persistentChangesAllowed
            onClicked: root.receiver.persistentChangesAllowed = checked
        }

        QGCNoteLabel {
            objectName: "rtkPersistentConsentWarning"
            text: (root._automatic ? qsTr("Applies only if a Quectel receiver is identified.") + " " : "")
                  + qsTr("For this connection only, allow QGroundControl to write requested base role or base-setting changes to receiver flash and restart it. Changes may remain saved even if reconnecting fails. No factory reset is performed. Permission is cleared after each attempt and is never used by automatic connections. To use the receiver as a rover again, restore its role with Quectel QGNSS or $PQTMCFGRCVRMODE,W,1 followed by $PQTMSAVEPAR.")
        }
    }

    Component.onCompleted: root._showIfRequired()

    // The permission is for the configuration shown here, so leaving the page withdraws it.
    Component.onDestruction: {
        if (root.receiver) {
            root.receiver.persistentChangesAllowed = false
        }
    }
}
