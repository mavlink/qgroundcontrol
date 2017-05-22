/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief On/Off Switch
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0

import TyphoonHQuickInterface       1.0

Item {
    id:     _root
    height: ScreenTools.defaultFontPixelHeight * 4
    width:  height

    property alias  text:       label.text
    property bool   calibrated: false

    property bool   valid:      TyphoonHQuickInterface.m4State === TyphoonHQuickInterface.M4_STATE_FACTORY_CAL || TyphoonHQuickInterface.m4State === TyphoonHQuickInterface.M4_STATE_AWAIT

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Rectangle {
        id:             indicator
        color:          valid ? (calibrated ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
        border.color:   qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
        border.width:   1
        anchors.fill:   parent
        QGCLabel {
            id:         label
            anchors.centerIn: parent
        }
    }
}
