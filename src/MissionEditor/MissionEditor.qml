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
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Mission Editor

QGCView {
    viewPanel: panel

    readonly property real  _defaultLatitude:   37.803784
    readonly property real  _defaultLongitude:  -122.462276
    readonly property int   _decimalPlaces:     7
    readonly property real  _horizontalMargin:   ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _verticalMargin:     ScreenTools.defaultFontPixelHeight / 2
    readonly property var   _activeVehicle:     multiVehicleManager.activeVehicle

    QGCPalette { id: _qgcPal; colorGroupEnabled: enabled }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Item {
            anchors.fill: parent

            FlightMap {
                id:             editorMap
                anchors.left:   parent.left
                anchors.right:  missionItemView.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                mapName:        "MissionEditor"
                latitude:       _defaultLatitude
                longitude:      _defaultLongitude

                QGCLabel {
                    anchors.right: parent.right
                    text: "WIP: Danger, do not fly with this!"; font.pixelSize: ScreenTools.largeFontPixelSize }


                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = 0
                        controller.addMissionItem(coordinate)
                    }
                }

                // Add the mission items to the map
                MapItemView {
                    model: controller.missionItems
                    
                    delegate:
                        MissionItemIndicator {
                            label:          object.sequenceNumber
                            isCurrentItem:  object.isCurrentItem
                            coordinate:     object.coordinate

                            Component.onCompleted: console.log("Indicator", object.coordinate)
                        }
                }

                Column {
                    id:                 controlWidgets
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.right:      parent.left
                    anchors.bottom:     parent.top
                    spacing:            ScreenTools.defaultFontPixelWidth / 2

                    QGCButton {
                        id:         addMode
                        text:       "+"
                        checkable:  true
                    }
                }
            } // FlightMap

            Rectangle {
                id:                 missionItemView
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              ScreenTools.defaultFontPixelWidth * 30
                color: _qgcPal.window

                Item {
                    anchors.margins:    _verticalMargin
                    anchors.fill:       parent

                    QGCButton {
                        id:     toolsButton
                        text:   "Tools"
                        menu :  toolMenu

                        Menu {
                            id: toolMenu

                            MenuItem {
                                text:       "Get mission items from vehicle"
                                enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                onTriggered: controller.getMissionItems()
                            }

                            MenuItem {
                                text:       "Send mission items to vehicle"
                                enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                onTriggered: controller.setMissionItems()
                            }

                            MenuItem {
                                text: "Load mission items from disk"
                            }

                            MenuItem {
                                text: "Save mission items to disk"
                            }
                        }
                    }

                    // Mission item list
                    ListView {
                        id:                 missionItemSummaryList
                        anchors.topMargin:  _verticalMargin
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.top:        toolsButton.bottom
                        anchors.bottom:     parent.bottom
                        spacing:            _verticalMargin
                        orientation:        ListView.Vertical
                        model:              controller.missionItems

                        property real _maxItemHeight: 0

                        delegate:
                            MissionItemEditor {
                                missionItem:    object
                                width:          parent.width
                            }
                    } // ListView
                } // Item
            } // Rectangle - mission item list
        } // Item - split view container
    } // QGCViewPanel
} // QGCVIew
