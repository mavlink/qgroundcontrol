/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             1.2

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import Custom.Widgets             1.0

Item {
    id:                 _root
    width:              background.width
    height:             background.height
    property real       pointSize:  ScreenTools.defaultFontPointSize
    property alias      text:       labelText.text
    property color      color:      "#FFF"
    property real       level:      0.5
    property string     title:      ""
    CustomTextBackground {
        id:                         background
        contentWidth:               labelRow.width
        contentHeight:              labelText.height * 2
        opacity:                    parent.level
    }
    Row {
        id:                         labelRow
        spacing:                    _root.title !== "" ? ScreenTools.defaultFontPixelWidth : 0
        anchors.centerIn:           background
        QGCLabel {
            id:                     labelTitle
            color:                  "#AAAAAA"
            text:                   _root.title
            visible:                _root.title !== ""
            font.pointSize:         _root.pointSize
        }
        QGCLabel {
            id:                     labelText
            color:                  _root.color
            font.pointSize:         _root.pointSize
        }
    }
}
