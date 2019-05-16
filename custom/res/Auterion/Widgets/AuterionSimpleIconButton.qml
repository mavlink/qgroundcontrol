/****************************************************************************
 *
 *   (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.11
import QtQuick.Controls             2.4

import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:                             button
    height:                         size
    width:                          size
    radius:                         2
    property alias  source:         _icon.source
    property bool   flat:           false
    property real   size:           ScreenTools.defaultFontPixelWidth * 3
    signal clicked()
    color:                          mouseArea.pressed ? qgcPal.buttonHighlight : (button.flat ? Qt.rgba(0,0,0,0) : qgcPal.button)
    QGCColoredImage {
        id:                         _icon
        height:                     ScreenTools.defaultFontPixelHeight * 0.5
        width:                      height
        sourceSize.height:          height
        fillMode:                   Image.PreserveAspectFit
        color:                      mouseArea.pressed ? qgcPal.buttonHighlightText : qgcPal.buttonText
        anchors.centerIn:           parent
    }
    // Process hover events
    MouseArea {
        id:                         mouseArea
        anchors.fill:               parent
        onClicked:                  button.clicked()
    }
}
