import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.GPS.NTRIP
import QGroundControl.GPS.RTK
import QGroundControl.PositionManager

SettingsPage {
    id: _root

    QGCTabBar {
        id: tabBar
        Layout.fillWidth: true

        QGCTabButton { text: qsTr("RTK Base Station") }
        QGCTabButton { text: qsTr("NTRIP") }
        QGCTabButton { text: qsTr("GCS Position") }
    }

    RTKSettings {
        Layout.fillWidth: true
        visible: tabBar.currentIndex === 0
    }

    NTRIPSettings {
        Layout.fillWidth: true
        visible: tabBar.currentIndex === 1
    }

    GCSPositionSettings {
        Layout.fillWidth: true
        visible: tabBar.currentIndex === 2
    }
}
