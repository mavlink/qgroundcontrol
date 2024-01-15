/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Controls
import QtQuick.Templates as T

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0

T.TabButton {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    font.pointSize: ScreenTools.defaultFontPointSize
    font.family:    ScreenTools.normalFontFamily

    padding: 6
    spacing: 6

    //icon.width: 24
    icon.height: ScreenTools.defaultFontPixelHeight
    icon.color: checked ? qgcPal.buttonHighlightText : qgcPal.buttonText


    contentItem: IconLabel {
        spacing: control.spacing
        mirrored: control.mirrored
        display: control.display

        icon: control.icon
        text: control.text
        font: control.font
        color: checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
    }

    background: Rectangle {
        implicitHeight: 40
        color: checked ? qgcPal.buttonHighlight : qgcPal.button
        /*color: Color.blend(control.checked ? control.qgcPal.window : control.qgcPal.dark,
                                             control.qgcPal.mid, control.down ? 0.5 : 0.0)*/
    }
}
