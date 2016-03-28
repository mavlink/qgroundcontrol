/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Main Tool Signal Strength
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4

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
        if (percent < 100)
            return "/qmlimages/Signal80.svg"
        return "/qmlimages/Signal100.svg"
    }

    QGCColoredImage {
        source:         getIcon()
        fillMode:       Image.PreserveAspectFit
        anchors.fill:   parent
        color:          qgcPal.buttonText
    }
}
