import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SettingsPage {
    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("Guide")
        headingDescription: qsTr("User guidance and documentation")

        Label {
            text: qsTr("See Help Settings for documentation and onboarding.")
            wrapMode: Text.WordWrap
        }
    }
}
