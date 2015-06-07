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
 *   @brief QGC HUD Message
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.4
import QtQuick.Controls 1.3

import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.MavManager 1.0

Item {
    id: root
    visible: MavManager.latestError !== ''
    Rectangle {
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        border.color:   Qt.rgba(1,1,1,0.75)
        radius:         4
        QGCLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter:   parent.verticalCenter
            antialiasing: true
            font.weight: Font.DemiBold
            text:  MavManager.latestError
            color: "#f84444"
        }
        OpacityAnimator {
            id: vanish
            target: root;
            from:   1;
            to:     0;
            duration: 2000
            running: false
        }
    }
    Timer {
        id: vanishTimer
        interval: 30000
        running:  false
        repeat:   false
        onTriggered: {
            vanish.start();
        }
    }
    MouseArea {
        anchors.fill: parent
        z: 1000
        acceptedButtons: Qt.LeftButton
        onClicked: {
            if (mouse.button == Qt.LeftButton)
            {
                vanishTimer.stop();
                vanish.stop();
                root.opacity = 0;
            }
        }
    }
    Connections {
        target: MavManager
        onLatestErrorChanged: {
            vanishTimer.stop();
            vanish.stop();
            vanishTimer.start();
            root.opacity = 1;
        }
    }
}
