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
    property real   size:       ScreenTools.defaultFontPixelWidth * 6
    signal          incremented ()
    signal          decremented ()
    property real   _spacers:   showValue ? ScreenTools.defaultFontPixelHeight * 0.5 : 2

    Column {
        id:                 valueCol
        spacing:            _spacers
        AuterionSimpleIconButton {
            size:           _root.size
            source:         "/auterion/img/plus_icon.svg"
            onClicked:      _root.incremented();
        }
        QGCLabel {
            id:             _label
            visible:        showValue
            font.pointSize: ScreenTools.smallFontPointSize
            anchors.horizontalCenter: parent.horizontalCenter
        }
        AuterionSimpleIconButton {
            size:           _root.size
            source:         "/auterion/img/minus_icon.svg"
            onClicked:      _root.decremented();
        }
    }
}
