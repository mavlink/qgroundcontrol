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

import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

Switch {
    id: control

    readonly property int _radius: 3

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  true
    }

    contentItem: QGCLabel {
        text:               control.text
        verticalAlignment:  Text.AlignVCenter
        rightPadding:       control.indicator.width + control.spacing
    }

    indicator: Rectangle {
        implicitWidth: knob.width * 2
        implicitHeight: knob.height
        x: control.width - width - control.rightPadding
        y: parent.height / 2 - height / 2
        radius: knob.radius
        color: control.checked ? qgcPal.primaryButton : qgcPal.button

        Rectangle {
            id: knob
            x: control.checked ? parent.width - width : 0
            width: ScreenTools.defaultFontPixelHeight
            height: ScreenTools.defaultFontPixelHeight
            radius: height / 2
            color: qgcPal.buttonText
        }
    }
}
