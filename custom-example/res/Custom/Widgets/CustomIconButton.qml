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
import QtGraphicalEffects           1.0

Button {
    id:                             _rootButton
    width:                          parent.height * 1.25
    height:                         parent.height
    flat:                           true
    contentItem: Item {
        id:                         _content
        anchors.fill:               _rootButton
        Row {
            id:                     _edge
            spacing:                ScreenTools.defaultFontPixelWidth * 0.25
            anchors.left:           parent.left
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model: [1,2,3]
                Rectangle {
                    height:         ScreenTools.defaultFontPixelHeight
                    width:          ScreenTools.defaultFontPixelWidth * 0.25
                    color:          qgcPal.text
                    opacity:        0.75
                }
            }
        }
        Image {
            id:                     _icon
            height:                 _rootButton.height * 0.75
            width:                  height
            smooth:                 true
            mipmap:                 true
            antialiasing:           true
            fillMode:               Image.PreserveAspectFit
            source:                 qgcPal.globalTheme === QGCPalette.Light ? "/res/QGCLogoBlack" : "/res/QGCLogoWhite"
            sourceSize.height:      height
            anchors.left:           _edge.right
            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    background: Item {
        anchors.fill: parent
    }
}
