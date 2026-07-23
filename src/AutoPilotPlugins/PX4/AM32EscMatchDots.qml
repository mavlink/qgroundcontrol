import QtQuick
import QGroundControl

Row {
    id: root

    required property string settingName
    required property var eeproms

    anchors.right: parent.right
    anchors.top: parent.top
    anchors.margins: 4
    spacing: 2
    visible: eeproms && eeproms.count > 1

    Repeater {
        model: root.eeproms ? root.eeproms : null

        Rectangle {
            property var esc: object
            property var setting: esc ? esc.settings[root.settingName] : null

            width: 8
            height: 8
            radius: 4
            color: {
                if (!esc || !esc.dataLoaded) return qgcPal.colorGrey
                if (!setting) return qgcPal.colorGrey
                return setting.matchesMajority ? qgcPal.colorGreen : qgcPal.colorRed
            }
        }
    }
}
