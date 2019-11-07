/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Layouts              1.11

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                 control
    height:             ScreenTools.defaultFontPixelHeight * 2
    autoExclusive:      true

    background: Rectangle {
        anchors.fill:   parent
        color:          checked ? qgcPal.buttonHighlight : qgcPal.button
    }

    property double messageHz:  0
    property int    compID:     0

    contentItem: RowLayout {
        QGCLabel {
            text:   control.compID
            color:  checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 3
        }
        QGCLabel {
            text:   control.text
            color:  checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            Layout.minimumWidth: ScreenTools.defaultFontPixelWidth * 28
        }
        QGCLabel {
            color:  checked ? qgcPal.buttonHighlightText : qgcPal.buttonText
            text:   messageHz.toFixed(1) + 'Hz'
            Layout.alignment: Qt.AlignRight
        }
    }
}
