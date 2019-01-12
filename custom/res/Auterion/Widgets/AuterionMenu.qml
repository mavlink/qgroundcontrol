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
import QtQuick.Controls.Styles      1.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id:     _root
    width:  labelBackground.width
    clip:   true

    property real       pointSize:      ScreenTools.normalFontPointSize
    property alias      text:           labelText.text
    property alias      model:          comboBox.model
    property int        currentIndex:   comboBox.currentIndex
    property real       level:          1

    readonly property real _dropImageWidth:    ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real _topBottomMargins:  ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real _dropImageMargin:   _dropImageWidth * 0.5

    Row {
        id:                         labelBackground
        spacing:                    0
        opacity:                    _root.level
        anchors.top:                parent.top
        anchors.topMargin:          _topBottomMargins
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       _topBottomMargins
        Image {
            source:                 "/auterion/img/label_left_edge.svg"
            width:                  height
            antialiasing:           true
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            fillMode:               Image.PreserveAspectFit
        }
        Rectangle {
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            width:                  menuRow.width * 1.15
            color:                  "#000000"
        }
        Image {
            source:                 "/auterion/img/label_right_edge.svg"
            width:                  height
            antialiasing:           true
            sourceSize.height:      height
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            fillMode:               Image.PreserveAspectFit
        }
    }
    Row {
        id:                         menuRow
        spacing:                    ScreenTools.defaultFontPixelWidth
        anchors.centerIn:           parent
        QGCLabel {
            id:                     labelText
            color:                  "#AAAAAA"
            font.pointSize:         _root.pointSize
            anchors.verticalCenter: parent.verticalCenter
        }
        AuterionComboBox {
            id:                     comboBox
            centeredLabel:          true
            pointSize:              _root.pointSize
        }
    }
}
