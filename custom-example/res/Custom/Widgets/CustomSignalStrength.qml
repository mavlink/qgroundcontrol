/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick                              2.11
import QtQuick.Controls                     1.4

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    width:  size
    height: size

    property real size:     50
    property real percent:  0

    QGCPalette { id: qgcPal }

    function getIcon() {
        if (percent < 20)
            return "/custom/img/menu_signal_0.svg"
        if (percent < 40)
            return "/custom/img/menu_signal_25.svg"
        if (percent < 60)
            return "/custom/img/menu_signal_50.svg"
        if (percent < 90)
            return "/custom/img/menu_signal_75.svg"
        return "/custom/img/menu_signal_100.svg"
    }

    QGCColoredImage {
        source:             getIcon()
        fillMode:           Image.PreserveAspectFit
        anchors.fill:       parent
        sourceSize.height:  size
        color:              qgcPal.text
    }
}
