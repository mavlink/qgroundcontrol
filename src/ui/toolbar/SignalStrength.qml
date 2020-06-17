/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Main Tool Signal Strength
 *   @author Gus Grubba <gus@auterion.com>
 */

import QtQuick 2.3

import QGroundControl.Controls  1.0
import QGroundControl.Palette   1.0

Item {
    id:     signalRoot
    width:  size
    height: size

    property real size:     50
    property real percent:  0

    QGCPalette { id: qgcPal }

    function getIcon() {
        if (percent < 20)
            return "/qmlimages/Signal0.svg"
        if (percent < 40)
            return "/qmlimages/Signal20.svg"
        if (percent < 60)
            return "/qmlimages/Signal40.svg"
        if (percent < 80)
            return "/qmlimages/Signal60.svg"
        if (percent < 95)
            return "/qmlimages/Signal80.svg"
        return "/qmlimages/Signal100.svg"
    }

    QGCColoredImage {
        source:             getIcon()
        fillMode:           Image.PreserveAspectFit
        anchors.fill:       parent
        color:              qgcPal.buttonText
        sourceSize.height:  size
    }
}
