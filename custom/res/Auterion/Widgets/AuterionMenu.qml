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
import QGroundControl.ScreenTools   1.0

import Auterion.Widgets             1.0

Item {
    id:                 _root
    width:              background.width
    height:             background.height

    property real       pointSize:      ScreenTools.normalFontPointSize
    property alias      text:           labelText.text
    property alias      model:          comboBox.model
    property int        currentIndex:   comboBox.currentIndex
    property real       level:          0.5

    AuterionTextBackground {
        id:                         background
        contentWidth:               menuRow.width
        contentHeight:              labelText.height * 2
        opacity:                    parent.level
    }
    Image {
        source:                     "/auterion/img/menu_dropdown.svg"
        height:                     background.height * 0.25
        width:                      height
        antialiasing:               true
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        anchors.right:              background.right
        anchors.rightMargin:        background.height * 0.25
        anchors.verticalCenter:     background.verticalCenter
    }
    Row {
        id:                         menuRow
        spacing:                    ScreenTools.defaultFontPixelWidth * 0.25
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
