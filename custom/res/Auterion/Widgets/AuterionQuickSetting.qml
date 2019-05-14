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

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

import Auterion.Widgets             1.0

Item {
    id:                 _root
    width:              valueCol.width
    height:             valueCol.height

    property bool   showValue:  false
    property alias  text:       _label.text
    signal          incremented ()
    signal          decremented ()
    property real   _spacers:   ScreenTools.defaultFontPixelHeight
    property real   _size:      ScreenTools.defaultFontPixelWidth * 4

    Column {
        id:                 valueCol
        spacing:            _spacers * 0.3333
        AuterionQuickSettingButton {
            icon.source:    "/auterion/img/plus_icon.svg"
            showArrow:      false
            iconRatio:      0.25
            flat:           false
            onClicked:      _root.incremented();
        }
        QGCLabel {
            id:             _label
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.horizontalCenter: parent.horizontalCenter
        }
        AuterionQuickSettingButton {
            icon.source:    "/auterion/img/minus_icon.svg"
            showArrow:      false
            iconRatio:      0.25
            flat:           false
            onClicked:      _root.decremented();
        }
    }
}
