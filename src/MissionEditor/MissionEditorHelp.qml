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

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

Rectangle {
    visible:            helpButton.checked
    color:              qgcPal.window
    opacity:            _rightPanelOpacity
    radius:             ScreenTools.defaultFontPixelHeight
    z:                  QGroundControl.zOrderTopMost

    readonly property real margins:  ScreenTools.defaultFontPixelHeight

    Image {
        anchors.margins:    ScreenTools.defaultFontPixelHeight
        anchors.top:        parent.top
        anchors.right:      parent.right
        width:              ScreenTools.defaultFontPixelHeight * 1.5
        height:             ScreenTools.defaultFontPixelHeight * 1.5
        source:             (qgcPal.globalTheme === QGCPalette.Light) ? "/res/XDeleteBlack.svg" : "/res/XDelete.svg"
        fillMode:           Image.PreserveAspectFit
        mipmap:             true
        smooth:             true

        MouseArea {
            anchors.fill:   parent
            onClicked:      helpButton.checked = false
        }
    }

    Item {
        anchors.margins:    _margin
        anchors.fill:       parent

        QGCLabel {
            id:             helpTitle
            font.pixelSize: ScreenTools.mediumFontPixelSize
            text:           "Mission Planner"
        }

        QGCLabel {
            id:                 helpIconLabel
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        helpTitle.bottom
            width:              parent.width
            wrapMode:           Text.WordWrap
            text:               "Mission Planner tool buttons:"
        }

        Image {
            id:                 addMissionItemsHelpIcon
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        helpIconLabel.bottom
            width:              ScreenTools.defaultFontPixelHeight * 3
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            source:             (qgcPal.globalTheme === QGCPalette.Light) ? "/qmlimages/MapAddMissionBlack.svg" : "/qmlimages/MapAddMission.svg"
        }

        QGCLabel {
            id:                 addMissionItemsHelpText
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
            anchors.left:       mapTypeHelpIcon.right
            anchors.right:      parent.right
            anchors.top:        addMissionItemsHelpIcon.top
            wrapMode:           Text.WordWrap
            text:               "<b>Add Mission Items</b><br>" +
                                "When enabled, add mission items by clicking on the map."
        }

        Image {
            id:                 mapCenterHelpIcon
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        addMissionItemsHelpText.bottom
            width:              ScreenTools.defaultFontPixelHeight * 3
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            source:             (qgcPal.globalTheme === QGCPalette.Light) ? "/qmlimages/MapCenterBlack.svg" : "/qmlimages/MapCenter.svg"
        }

        QGCLabel {
            id:                 mapCenterHelpText
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
            anchors.left:       mapTypeHelpIcon.right
            anchors.right:      parent.right
            anchors.top:        mapCenterHelpIcon.top
            wrapMode:           Text.WordWrap
            text:               "<b>Map Center</b><br>" +
                                "Options for centering the map."
        }

        Image {
            id:                 syncHelpIcon
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        mapCenterHelpText.bottom
            width:              ScreenTools.defaultFontPixelHeight * 3
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            source:             (qgcPal.globalTheme === QGCPalette.Light) ? "/qmlimages/MapSyncBlack.svg" : "/qmlimages/MapSync.svg"
        }

        QGCLabel {
            id:                 syncHelpText
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
            anchors.left:       mapTypeHelpIcon.right
            anchors.right:      parent.right
            anchors.top:        syncHelpIcon.top
            wrapMode:           Text.WordWrap
            text:               "<b>Sync</b><br>" +
                                "Options for saving/loading mission items."
        }

        Image {
            id:                 mapTypeHelpIcon
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        syncHelpText.bottom
            width:              ScreenTools.defaultFontPixelHeight * 3
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            source:             (qgcPal.globalTheme === QGCPalette.Light) ? "/qmlimages/MapTypeBlack.svg" : "/qmlimages/MapType.svg"
        }

        QGCLabel {
            id:                 mapTypeHelpText
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
            anchors.left:       mapTypeHelpIcon.right
            anchors.right:      parent.right
            anchors.top:        mapTypeHelpIcon.top
            wrapMode:           Text.WordWrap
            text:               "<b>Map Type</b><br>" +
                                "Map type options."
        }

        QGCCheckBox {
            anchors.left:       parent.left
            anchors.bottom:     parent.bottom
            anchors.margins:    _margin
            checked:            !_showHelp
            text:               "Don't show me again"

            onClicked:          QGroundControl.flightMapSettings.saveBoolMapSetting(editorMap.mapName, _showHelpKey, !checked)
        }
    }
}
