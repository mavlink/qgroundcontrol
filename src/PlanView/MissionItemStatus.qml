/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

Rectangle {
    height:     ScreenTools.defaultFontPixelHeight * 7
    radius:     ScreenTools.defaultFontPixelWidth * 0.5
    color:      qgcPal.window
    opacity:    0.80
    clip:       true

    property var missionItems                ///< List of all available mission items

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                     label
        anchors.top:            parent.bottom
        width:                  parent.height
        text:                   qsTr("Terrain Altitude")
        horizontalAlignment:    Text.AlignHCenter
        rotation:               -90
        transformOrigin:        Item.TopLeft
    }

    QGCListView {
        id:                     statusListView
        anchors.margins:        _margins
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.leftMargin:     ScreenTools.defaultFontPixelHeight
        anchors.left:           parent.left
        anchors.right:          parent.right
        model:                  missionItems
        highlightMoveDuration:  250
        orientation:            ListView.Horizontal
        spacing:                0
        clip:                   true
        currentIndex:           _currentMissionIndex

        delegate: Item {
            height:     statusListView.height
            width:      display ? (indicator.width + spacing)  : 0
            visible:    display

            property real availableHeight:  height - indicator.height
            property bool showTerrain:      !isNaN(object.terrainPercent)
            property real _terrainPercent:  showTerrain ? object.terrainPercent : 0

            readonly property bool display: object.specifiesCoordinate && !object.isStandaloneCoordinate
            readonly property real spacing: ScreenTools.defaultFontPixelWidth * ScreenTools.smallFontPointRatio

            Rectangle {
                anchors.bottom:             parent.bottom
                anchors.horizontalCenter:   parent.horizontalCenter
                width:                      indicator.width
                height:                     Math.max(availableHeight * _terrainPercent, 1)
                color:                      _terrainPercent > object.altPercent ? "red": qgcPal.text
                visible:                    !isNaN(object.terrainPercent)
            }

            MissionItemIndexLabel {
                id:                         indicator
                anchors.horizontalCenter:   parent.horizontalCenter
                y:                          availableHeight - (availableHeight * object.altPercent)
                small:                      true
                checked:                    object.isCurrentItem
                label:                      object.abbreviation.charAt(0)
                index:                      object.abbreviation.charAt(0) > 'A' && object.abbreviation.charAt(0) < 'z' ? -1 : object.sequenceNumber
                visible:                    true
            }
        }
    }
}


