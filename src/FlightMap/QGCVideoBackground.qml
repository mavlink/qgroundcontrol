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
 *   @brief QGC Video Background
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtQuick.Controls 1.3
import QGroundControl.QgcQtGStreamer 1.0

VideoItem {
    id: videoBackground
    property var display
    property var receiver
    property var runVideo:  false
    surface: display
    onRunVideoChanged: {
        if(videoBackground.receiver && videoBackground.display) {
            if(videoBackground.runVideo) {
                videoBackground.receiver.start();
            } else {
                videoBackground.receiver.stop();
            }
        }
    }
    Component.onCompleted: {
        if(videoBackground.runVideo && videoBackground.receiver) {
            videoBackground.receiver.start();
        }
    }
}
