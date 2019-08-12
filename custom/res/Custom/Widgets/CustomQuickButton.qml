/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Button {
    id:                         control
    autoExclusive:              true
    checkable:                  true

    property string iconSource:     ""
    property real   iconRatio:      0.5
    property real   buttonRadius:   ScreenTools.defaultFontPixelWidth * 0.5

    background: Rectangle {
        width:                  control.width
        height:                 width
        anchors.centerIn:       parent
        color:                  (mouseArea.pressed || control.checked) ? qgcPal.buttonHighlight : (qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.5) : Qt.rgba(0,0,0,0.5))
        radius:                 control.buttonRadius
    }
    contentItem: Item {
        anchors.fill:           control
        QGCColoredImage {
            source:             iconSource
            color:              (mouseArea.pressed || control.checked) ? qgcPal.buttonHighlightText : qgcPal.buttonText
            width:              control.width * iconRatio
            height:             width
            anchors.centerIn:   parent
            sourceSize.height:  height
        }
    }
    MouseArea {
        id:                     mouseArea
        anchors.fill:           parent
        onClicked: {
            if(checkable)
                checked = true
            control.clicked()
        }
    }
}
