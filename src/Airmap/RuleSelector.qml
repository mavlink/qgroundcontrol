import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Airmap            1.0
import QGroundControl.SettingsManager   1.0

Button {
    id:                         _root
    autoExclusive:              false
    height:                     ScreenTools.defaultFontPixelHeight
    background: Rectangle {
        anchors.fill:           parent
        color:                  _selected ? qgcPal.windowShade : qgcPal.window
    }
    property var    rule:       null
    property bool   _selected: {
        if (autoExclusive) {
            return checked
        } else {
            return rule ? rule.selected : false
        }
    }
    onCheckedChanged: {
        rule.selected = checked
    }
    contentItem: Row {
        id:             ruleRow
        spacing:        ScreenTools.defaultFontPixelWidth
        anchors.right:  parent.right
        anchors.left:   parent.left
        anchors.verticalCenter: parent.verticalCenter
        Rectangle {
            width:      ScreenTools.defaultFontPixelWidth * 0.75
            height:     ScreenTools.defaultFontPixelHeight
            color:      _selected ? qgcPal.colorGreen : qgcPal.window
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            text:           rule ? (rule.name === "" ? rule.shortName : rule.name) : ""
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    onClicked: {
        if (autoExclusive) {
            checked = true
        } else {
            rule.selected = !rule.selected
        }
    }
}
