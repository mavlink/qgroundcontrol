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
 *   @brief QGC Main Tool GPS
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QGroundControl.Controls 1.0

Item {
    id:     gpsRoot
    property real size:     50
    property real percent:  0
    width:  size
    height: size
    Image {
        source:         "/qmlimages/Gps.svg"
        fillMode:       Image.PreserveAspectFit
        mipmap:         true
        smooth:         true
        anchors.fill:   parent
        opacity:        (percent + 25) * 0.8
    }
}
