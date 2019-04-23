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

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

import AuterionQuickInterface       1.0

Item {
    id:                         _root
    width:                      edge.width + icon.width + sectionTitle.width
    height:                     parent.height
    property alias text:        sectionTitle.text
    signal clicked()
    Image {
        id:                     edge
        height:                 ScreenTools.defaultFontPixelHeight
        width:                  height
        sourceSize.height:      parent.height
        fillMode:               Image.PreserveAspectFit
        source:                 "/auterion/img/menu_left_edge.svg"
        anchors.left:           parent.left
        anchors.verticalCenter: parent.verticalCenter
    }
    Image {
        id:                     icon
        height:                 parent.height
        width:                  height
        sourceSize.height:      parent.height
        fillMode:               Image.PreserveAspectFit
        source:                 QGroundControl.corePlugin.showAdvancedUI ? "/auterion/img/menu_logo_advanced.svg" : "/auterion/img/menu_logo.svg"
        anchors.left:           edge.right
    }
    QGCLabel {
        id:                     sectionTitle
        anchors.verticalCenter: parent.verticalCenter
        anchors.left:           icon.right
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            _root.clicked()
        }
    }
}
