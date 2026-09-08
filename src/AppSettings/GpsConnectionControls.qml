import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight / 2

    required property var autoConnectFact
    required property bool active
    required property string statusText
    property bool available: true
    property string autoConnectObjectName: ""
    property string connectButtonObjectName: ""
    property string statusObjectName: ""

    signal connectRequested()
    signal disconnectRequested()

    FactCheckBox {
        objectName: root.autoConnectObjectName
        Layout.fillWidth: true
        text: qsTr("Connect automatically")
        fact: root.autoConnectFact
        visible: fact.userVisible
        enabled: root.available
        onClicked: root.forceActiveFocus()
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        wrapMode: Text.WordWrap
        visible: root.autoConnectFact.rawValue && root.available
        text: qsTr("Connects at startup and reconnects when needed. Disconnect pauses autoconnect until you click Connect, re-enable this option, or restart the application.")
    }

    QGCLabel {
        objectName: root.statusObjectName
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        wrapMode: Text.WordWrap
        text: root.statusText
    }

    QGCButton {
        objectName: root.connectButtonObjectName
        text: root.active ? qsTr("Disconnect") : qsTr("Connect")
        enabled: root.active || root.available
        onClicked: {
            root.forceActiveFocus()
            if (root.active) {
                root.disconnectRequested()
            } else {
                root.connectRequested()
            }
        }
    }
}
