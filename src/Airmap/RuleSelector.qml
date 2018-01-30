import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.Airmap            1.0
import QGroundControl.SettingsManager   1.0

Rectangle {
    id:                         _root
    height:                     ScreenTools.defaultFontPixelHeight
    color:                      _selected ? qgcPal.windowShade : qgcPal.window
    property var    rule:       null
    property bool   checked:    false
    property bool   required:   false
    property bool   _selected: {
        if (exclusiveGroup) {
            return checked
        } else {
            return rule ? rule.selected : false
        }
    }
    property ExclusiveGroup exclusiveGroup:  null
    onExclusiveGroupChanged: {
        if (exclusiveGroup) {
            checked = rule.selected
            exclusiveGroup.bindCheckable(_root)
        }
    }
    onCheckedChanged: {
        rule.selected = checked
    }
    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }
    Row {
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
            text:       rule.name === "" ? rule.shortName : rule.name
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    MouseArea {
        anchors.fill:   parent
        enabled:        !required
        onClicked: {
            if (exclusiveGroup) {
                checked = true
            } else {
                rule.selected = !rule.selected
            }
        }
    }
}
