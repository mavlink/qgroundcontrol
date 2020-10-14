import QtQuick                      2.12
import QtQuick.Controls             2.12
import QtQuick.Controls.impl        2.12
import QtQml                        2.12

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

TabButton {
    id:             control
    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily
    icon.color:     _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText

    property bool _showHighlight: (pressed | hovered | checked)

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    contentItem: IconLabel {
        spacing:    control.spacing
        mirrored:   control.mirrored
        display:    control.display
        icon:       control.icon
        font:       control.font
        color:      _showHighlight ? qgcPal.buttonHighlightText : qgcPal.buttonText
    }

    background: Rectangle {
        color: _showHighlight ? qgcPal.buttonHighlight : qgcPal.button
    }
}
