import QtQuick

import QGroundControl

/// Pill showing the most severe level: Error, then Warning, then Info
StatusPill {
    text: _style ? _style.text : ""
    color: _style ? _style.color : "transparent"

    property int errorCount: 0
    property int warningCount: 0
    property int unreadCount: 0
    property string warningLabel: qsTr("Warning")

    readonly property string severity: {
        if (errorCount > 0) {
            return "error"
        }
        if (warningCount > 0) {
            return "warning"
        }
        if (unreadCount > 0) {
            return "info"
        }
        return ""
    }

    readonly property var _palette: QGroundControl.globalPalette
    readonly property var _styles: ({
        error: { text: qsTr("Error"), color: _palette.errorIndicator },
        warning: { text: warningLabel, color: _palette.warningIndicator },
        info: { text: qsTr("Info"), color: _palette.colorBlue }
    })
    readonly property var _style: _styles[severity]
}
