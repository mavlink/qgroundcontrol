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
