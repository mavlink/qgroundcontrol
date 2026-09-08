import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: root

    required property GPSSourceHealth health
    property string satelliteStatusObjectName: ""
    property string fixStatusObjectName: ""

    QGCLabel {
        objectName: root.fixStatusObjectName
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: {
            if (!root.health || root.health.state === GPSSourceHealth.NoData) return qsTr("Position: waiting for a fix")
            if (root.health.state === GPSSourceHealth.Stale) return qsTr("Position: stale")
            if (!root.health.usable) return qsTr("Position: no usable fix")
            return qsTr("Position: usable (horizontal accuracy %1 m)").arg(root.health.horizontalAccuracy.toFixed(1))
        }
    }

    QGCLabel {
        objectName: root.satelliteStatusObjectName
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Satellites: %1 in use / %2 in view")
            .arg(root.health && root.health.satellitesInUseCount >= 0 ? root.health.satellitesInUseCount : qsTr("Unknown"))
            .arg(root.health && root.health.satellitesInViewCount >= 0 ? root.health.satellitesInViewCount : qsTr("Unknown"))
    }
}
