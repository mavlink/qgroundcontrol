/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:     _linkRoot
    color:  __qgcPal.window

    property var _currentSelection: null

    ExclusiveGroup { id: linkGroup }

    QGCPalette {
        id:                 qgcPal
        colorGroupEnabled:  enabled
    }

    Flickable {
        clip:               true
        anchors.top:        parent.top
        width:              parent.width
        height:             parent.height - buttonRow.height
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      settingsColumn.height
        contentWidth:       _linkRoot.width
        flickableDirection: Flickable.VerticalFlick
        boundsBehavior:     Flickable.StopAtBounds

        Column {
            id:                 settingsColumn
            width:              _linkRoot.width
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight / 2
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            QGCLabel {
                text:   "WIP: Not fully functional"
                color:  __qgcPal.warningText
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  parent.width
            }
            Repeater {
                model: QGroundControl.linkManager.linkConfigurations
                delegate:
                QGCButton {
                    text:   object.name
                    width:  _linkRoot.width * 0.5
                    exclusiveGroup: linkGroup
                    anchors.horizontalCenter: parent.horizontalCenter
                    onClicked: {
                        checked = true
                        _currentSelection = object
                    }
                }
            }
        }
    }

    Row {
        id:                 buttonRow
        spacing:            ScreenTools.defaultFontPixelWidth
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.horizontalCenter: parent.horizontalCenter
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Delete"
            enabled:    false
            onClicked: {
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Edit"
            enabled:    false
            onClicked: {
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Add"
            enabled:    false
            onClicked: {
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Connect"
            enabled:    _currentSelection && !_currentSelection.link
            onClicked: {
                QGroundControl.linkManager.createConnectedLink(_currentSelection)
                settingsMenu.closeSettings()
            }
        }
        QGCButton {
            width:      ScreenTools.defaultFontPixelWidth * 10
            text:       "Disconnect"
            enabled:    _currentSelection && _currentSelection.link
            onClicked: {
                QGroundControl.linkManager.disconnectLink(_currentSelection.link, false)
            }
        }
    }
}
