/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtLocation
import QtQuick.Controls

import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.Vehicle
import QGroundControl.Palette

/// Marker for displaying a camera trigger on the map
MapQuickItem {
    anchorPoint.x:  sourceItem.width / 2
    anchorPoint.y:  sourceItem.height / 2

    sourceItem: Rectangle {
        width:      _radius * 2
        height:     _radius * 2
        radius:     _radius
        color:      Qt.rgba(0.045, 0.048, 0.052, 0.58)

        readonly property real _radius: ScreenTools.defaultFontPixelHeight * 0.6

        QGCPalette { id: qgcPal; colorGroupEnabled: true }

        // Bigger rectangle of camera icon
        Rectangle {
            id:                       cameraIconFrameRectangle
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter:   parent.verticalCenter
            height:                   width * 0.6
            width:                    parent.width * 0.6
            color:                    qgcPal.text
            radius:                   4

            // Little rectangle on top indicating viewfinder
            Rectangle {
                id:                       cameraIconViewFinderRectangle
                anchors.horizontalCenter: cameraIconFrameRectangle.horizontalCenter
                anchors.bottom:           cameraIconFrameRectangle.top
                width:                    cameraIconFrameRectangle.width * 0.5
                height:                   cameraIconFrameRectangle.height * 0.3
                color:                    qgcPal.text
                radius:                   2
            }
        }

        // Circunference indicating the lens
        Rectangle {
            id:                      cameraIconLens
            anchors.centerIn:        cameraIconFrameRectangle
            height:                  width
            width:                   cameraIconFrameRectangle.height * 0.9
            color:                   "transparent"
            border.color:            Qt.rgba(0.045, 0.048, 0.052, 0.88)
            opacity:                 0.9
            border.width:            2
            radius:                  width * 0.5
        }
    }
}
