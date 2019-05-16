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
import QtGraphicalEffects           1.0


import AuterionQuickInterface       1.0

Item {
    id:                         _root
    width:                      edge.width + icon.width
    height:                     parent.height
    signal clicked()
    QGCColoredImage {
        id:                     edge
        height:                 ScreenTools.defaultFontPixelHeight
        width:                  height
        sourceSize.height:      parent.height
        fillMode:               Image.PreserveAspectFit
        source:                 "/auterion/img/menu_left_edge.svg"
        color:                  qgcPal.text
        anchors.left:           parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
    }

    Image {
        id:                     icon
        height:                 parent.height
        width:                  height
        smooth:                 true
        mipmap:                 true
        antialiasing:           true
        visible:                !QGroundControl.corePlugin.showAdvancedUI && qgcPal.globalTheme === QGCPalette.Dark
        fillMode:               Image.PreserveAspectFit
        anchors.left:           edge.right
        source:                 QGroundControl.corePlugin.showAdvancedUI ? "/auterion/img/menu_logo_advanced.svg" : "/auterion/img/menu_logo.svg"
        sourceSize.height:      height
    }

    ColorOverlay {
        anchors.fill:   icon
        source:         icon
        color:          QGroundControl.corePlugin.showAdvancedUI  ? qgcPal.text : (qgcPal.globalTheme === QGCPalette.Dark ? "black" : qgcPal.text )
        visible:        !icon.visible
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            _root.clicked()
        }
    }
}
