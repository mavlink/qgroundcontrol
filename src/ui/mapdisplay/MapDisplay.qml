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
 *   @brief QGC Main Map Display
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Layouts 1.1

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.FlightControls 1.0

Rectangle {
    QGCPalette { id: __palette; colorGroupEnabled: true }
    id: root
    color: Qt.rgba(0,0,0,0);

    property real roll:    isNaN(mapEngine.roll)    ? 0 : mapEngine.roll
    property real pitch:   isNaN(mapEngine.pitch)   ? 0 : mapEngine.pitch
    property bool showWaypointEditor: true

    SplitView {
        id:           splitView
        anchors.fill: parent
        orientation:  Qt.Horizontal
        z:            10

        // This sets the color of the splitter line
        handleDelegate: Rectangle {
                width:  1
                height: 1
                color:  __palette.window
            }

        //----------------------------------------------------------------------------------------
        // Map View
        QGCMapBackground {
            id:                     mapBackground
            Layout.fillWidth:       true
            Layout.minimumWidth:    300
            heading:                isNaN(mapEngine.heading) ? 0 : mapEngine.heading
            latitude:               37.803784   // mapEngine.latitude
            longitude:              -122.462276 // mapEngine.longitude
            showWaypoints:          true
            mapName:                'MainMapDisplay'

            // Chevron button at upper right corner of Map Display
            Item {
                id:             openWaypoints
                anchors.top:    mapBackground.top
                anchors.right:  mapBackground.right
                width:          30
                height:         30
                opacity:        0.85
                z:              splitView.z + 10
                Image {
                    id:             buttomImg
                    anchors.fill:   parent
                    source:         showWaypointEditor ? "/qmlimages/buttonRight.svg" : "/qmlimages/buttonLeft.svg"
                    mipmap:         true
                    smooth:         true
                    antialiasing:   true
                    fillMode:       Image.PreserveAspectFit
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        if (mouse.button == Qt.LeftButton)
                        {
                            showWaypointEditor = !showWaypointEditor
                        }
                    }
                }
            }
        }
        //----------------------------------------------------------------------------------------
        // Waypoint Editor
        QGCWaypointEditor {
            id:                  waypointEditor
            Layout.minimumWidth: 200
            visible:             showWaypointEditor
        }
    }

    //--------------------------------------------------------------------------------------------
    // Right click anywhere on the map for a context menu
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: {
            if (mouse.button == Qt.RightButton)
            {
                mapBackground.mapMenu.popup()
            }
        }
        z: splitView.z + 5
    }

    //--------------------------------------------------------------------------------------------
    // Tool Bar
    Rectangle {
        id:                     toolBar
        color:                  Qt.rgba(0,0,0,0)
        height:                 buttonColumn.height
        visible:                showWaypointEditor
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.topMargin:      40
        anchors.leftMargin:     4
        z:                      splitView.z + 10

        ExclusiveGroup { id: mainActionGroup }

        Column {
            id: buttonColumn
            spacing: 4
            QGCMapToolButton {
                width:  50
                height: 50
                imageSource: "/qmlimages/buttonHome.svg"
                exclusiveGroup: mainActionGroup
            }
            QGCMapToolButton {
                width:  50
                height: 50
                imageSource: "/qmlimages/buttonHome.svg"
                exclusiveGroup: mainActionGroup
            }
            QGCMapToolButton {
                width:  50
                height: 50
                imageSource: "/qmlimages/buttonHome.svg"
                exclusiveGroup: mainActionGroup
            }
            QGCMapToolButton {
                width:  50
                height: 50
                imageSource: "/qmlimages/buttonHome.svg"
                exclusiveGroup: mainActionGroup
            }
        }
    }
}
