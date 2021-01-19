/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                      2.11
import QtQuick.Controls             2.4
import QtQuick.Dialogs              1.3
import QtQuick.Layouts              1.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Item {
    width:                  availableWidth
    height:                 mainRect.height + (ScreenTools.defaultFontPixelHeight * 2)
    Rectangle {
        id:                 mainRect
        y:                  ScreenTools.defaultFontPixelHeight * 0.5
        width:              parent.width * 0.9
        height:             panelLoader.height * 0.8
        color:              "#364c7f"
        anchors.horizontalCenter: parent.horizontalCenter
    }
}


