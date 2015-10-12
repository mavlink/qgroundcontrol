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

import QGroundControl               1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Mission Editor

QGCView {
    viewPanel: panel

    readonly property int   _decimalPlaces:     7
    readonly property real  _horizontalMargin:  ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _verticalMargin:    ScreenTools.defaultFontPixelHeight / 2
    readonly property var   _activeVehicle:     multiVehicleManager.activeVehicle
    readonly property real  _editFieldWidth:    ScreenTools.defaultFontPixelWidth * 16

    property var    _missionItems:              controller.missionItems
    property bool   _showHomePositionManager:   false
    property bool   _addMissionItems:           false

    property var    _homePositionManager:       QGroundControl.homePositionManager
    property string _homePositionName:          _homePositionManager.homePositions.get(0).name
    property var    _homePositionCoordinate:    _homePositionManager.homePositions.get(0).coordinate

    QGCPalette { id: _qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    function setCurrentItem(index) {
        for (var i=0; i<_missionItems.count; i++) {
            _missionItems.get(i).isCurrentItem = (i == index)
        }
    }

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

                Component.onCompleted: {
                    latitude = _homePositionCoordinate.latitude
                    longitude = _homePositionCoordinate.longitude
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                        if (_showHomePositionManager) {
                            _homePositionCoordinate = coordinate
                        } else if (_addMissionItems) {
                            var index = controller.addMissionItem(coordinate)
                            setCurrentItem(index)
                        }
                    }
                }

                Rectangle {
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.bottom:             parent.bottom
                    width:                      parent.width / 3
                    height:                     syncNeededText.height + (ScreenTools.defaultFontPixelWidth * 2)
                    border.width:               1
                    border.color:               "white"
                    color:                      "black"
                    opacity:                    0.75
                    visible:                    controller.missionItems.dirty

                    QGCLabel {
                        id:                     syncNeededText
                        anchors.margins:        ScreenTools.defaultFontPixelWidth
                        anchors.top:            parent.top
                        anchors.left:           parent.left
                        anchors.right:          parent.right
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.pixelSize:         ScreenTools.mediumFontPixelSize
                        text:                   "You have unsaved changes. Be sure to use the Sync tool to save when ready."
                    }
                }

                Row {
                    spacing:            ScreenTools.defaultFontPixelWidth
                    anchors.top:        parent.top
                    anchors.right:      parent.right
                    anchors.margins:    ScreenTools.defaultFontPixelWidth

                    RoundButton {
                        id:                     addMissionItemsButton
                        buttonImage:            "/qmlimages/MapAddMission.svg"
                        opacity:                _addMissionItems ? 1.0 : 0.75
                        onClicked: {
                            _addMissionItems = !_addMissionItems
                            _showHomePositionManager = false
                        }
                    }

                    RoundButton {
                        id:                     homePositionManagerButton
                        buttonImage:            "/qmlimages/MapHome.svg"
                        opacity:                _showHomePositionManager ? 1.0 : 0.75
                        onClicked: {
                            _showHomePositionManager = !_showHomePositionManager
                            _addMissionItems = false
                        }
                    }

                    DropButton {
                        id:                     centerMapButton
                        dropDirection:          dropDown
                        buttonImage:            "/qmlimages/MapCenter.svg"
                        viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:         _dropButtonsExclusiveGroup

                        dropDownComponent: Component {
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text: "Home"

                                    onClicked: {
                                        centerMapButton.hideDropDown()
                                        editorMap.center = QtPositioning.coordinate(_homePositionCoordinate.latitude, _homePositionCoordinate.longitude)
                                        _showHomePositionManager = true
                                    }
                                }

                                QGCButton {
                                    text:       "Vehicle"
                                    enabled:    activeVehicle && activeVehicle.latitude != 0 && activeVehicle.longitude != 0

                                    property var activeVehicle: multiVehicleManager.activeVehicle

                                    onClicked: {
                                        centerMapButton.hideDropDown()
                                        editorMap.latitude = activeVehicle.latitude
                                        editorMap.longitude = activeVehicle.longitude
                                    }
                                }

    /*

    This code will need to wait for Qml 5.5 support since Map.visibleRegion is only in Qt 5.5

                                QGCButton {
                                    text: "All Items"

                                    onClicked: {
                                        centerMapButton.hideDropDown()

                                        // Begin with only the home position in the region
                                        var region = QtPositioning.rectangle(QtPositioning.coordinate(_homePositionCoordinate.latitude, _homePositionCoordinate.longitude),
                                                                             QtPositioning.coordinate(_homePositionCoordinate.latitude, _homePositionCoordinate.longitude))

                                        // Now expand the region to include all mission items
                                        for (var i=0; i<_missionItems.count; i++) {
                                            var missionItem = _missionItems.get(i)

                                            region.topLeft.latitude = Math.max(missionItem.coordinate.latitude, region.topLeft.latitude)
                                            region.topLeft.longitude = Math.min(missionItem.coordinate.longitude, region.topLeft.longitude)

                                            region.topRight.latitude = Math.max(missionItem.coordinate.latitude, region.topRight.latitude)
                                            region.topRight.longitude = Math.max(missionItem.coordinate.longitude, region.topRight.longitude)

                                            region.bottomLeft.latitude = Math.min(missionItem.coordinate.latitude, region.bottomLeft.latitude)
                                            region.bottomLeft.longitude = Math.min(missionItem.coordinate.longitude, region.bottomLeft.longitude)

                                            region.bottomRight.latitude = Math.min(missionItem.coordinate.latitude, region.bottomRight.latitude)
                                            region.bottomRight.longitude = Math.max(missionItem.coordinate.longitude, region.bottomRight.longitude)
                                        }

                                        editorMap.visibleRegion = region
                                    }
                                }
    */
                            }
                        }
                    }

                    DropButton {
                        id:                     syncButton
                        dropDirection:          dropDown
                        buttonImage:            "/qmlimages/MapSync.svg"
                        viewportMargins:        ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:         _dropButtonsExclusiveGroup

                        dropDownComponent: Component {
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text:       "Load from vehicle"
                                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        controller.getMissionItems()
                                    }
                                }

                                QGCButton {
                                    text:       "Save to vehicle"
                                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        controller.setMissionItems()
                                    }
                                }

                                QGCButton {
                                    text:       "Load from file..."

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        controller.loadMissionFromFile()
                                    }
                                }

                                QGCButton {
                                    text:       "Save to file..."

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        controller.saveMissionToFile()
                                    }
                                }
                            }
                        }
                    }

                    DropButton {
                        id:                 mapTypeButton
                        dropDirection:      dropDown
                        buttonImage:        "/qmlimages/MapType.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:         _dropButtonsExclusiveGroup

                        dropDownComponent: Component {
                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                Repeater {
                                    model: QGroundControl.flightMapSettings.mapTypes

                                    QGCButton {
                                        checkable:      true
                                        checked:        editorMap.mapType == text
                                        text:           modelData
                                        exclusiveGroup: _mapTypeButtonsExclusiveGroup

                                        onClicked: {
                                            editorMap.mapType = text
                                            checked = true
                                            mapTypeButton.hideDropDown()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                MissionItemIndicator {
                    label:          "H"
                    isCurrentItem:  _showHomePositionManager
                    coordinate:     _homePositionCoordinate
                    z:              2

                    onClicked: _showHomePositionManager = true
                }

                // Add the mission items to the map
                MapItemView {
                    model: controller.missionItems
                    
                    delegate:
                        MissionItemIndicator {
                            label:          object.sequenceNumber
                            isCurrentItem:  !_showHomePositionManager && object.isCurrentItem
                            coordinate:     object.coordinate
                            z:              2

                            onClicked: {
                                _showHomePositionManager = false
                                setCurrentItem(object.sequenceNumber)
                            }
                        }
                }

                MapPolyline {
                    id:         homePositionLine
                    line.width: 3
                    line.color: "orange"
                    z:          1

                    property var homePositionCoordinate: _homePositionCoordinate

                    function update() {
                        while (homePositionLine.path.length != 0) {
                            homePositionLine.removeCoordinate(homePositionLine.path[0])
                        }
                        if (_missionItems && _missionItems.count != 0) {
                            homePositionLine.addCoordinate(homePositionCoordinate)
                            homePositionLine.addCoordinate(_missionItems.get(0).coordinate)
                        }
                    }

                    onHomePositionCoordinateChanged: update()

                    Connections {
                        target: controller

                        onWaypointLinesChanged: homePositionLine.update()
                    }

                    Component.onCompleted: homePositionLine.update()
                }


                // Add lines between waypoints
                MapItemView {
                    model: controller.waypointLines

                    delegate:
                        MapPolyline {
                            line.width: 3
                            line.color: "orange"
                            z:          1

                            path: [
                                { latitude: object.coordinate1.latitude, longitude: object.coordinate1.longitude },
                                { latitude: object.coordinate2.latitude, longitude: object.coordinate2.longitude },
                            ]
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

                    // Mission Item Editor
                    Item {
                        anchors.fill:   parent
                        visible:        !_showHomePositionManager && controller.missionItems.count != 0

                        ListView {
                            id:             missionItemSummaryList
                            anchors.fill:   parent
                            spacing:        _verticalMargin
                            orientation:    ListView.Vertical
                            model:          controller.canEdit ? controller.missionItems : 0

                            property real _maxItemHeight: 0

                            delegate:
                                MissionItemEditor {
                                    missionItem:    object
                                    width:          parent.width

                                    onClicked:  setCurrentItem(object.sequenceNumber)

                                    onRemove: {
                                        var newCurrentItem = object.sequenceNumber - 1
                                        controller.removeMissionItem(object.sequenceNumber)
                                        if (_missionItems.count) {
                                            newCurrentItem = Math.min(_missionItems.count - 1, newCurrentItem)
                                            setCurrentItem(newCurrentItem)
                                        }
                                    }

                                    onMoveUp:   controller.moveUp(object.sequenceNumber)
                                    onMoveDown: controller.moveDown(object.sequenceNumber)
                                }
                        } // ListView

                        QGCLabel {
                            anchors.fill:   parent
                            visible:        !controller.canEdit
                            wrapMode:       Text.WordWrap
                            text:           "The set of mission items you have loaded cannot be edited by QGroundControl. " +
                                            "You will only be able to save these to a file, or send them to a vehicle."
                        }
                    } // Item - Mission Item editor

                    // Home Position Manager
                    Item {
                        anchors.fill:   parent
                        visible:        _showHomePositionManager

                        Column {
                            anchors.fill: parent

                            QGCLabel {
                                font.pixelSize: ScreenTools.mediumFontPixelSize
                                text:           "Home Position Manager"
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight
                            }

                            QGCLabel {
                                text: "Select home position to use:"
                            }

                            QGCComboBox {
                                id:         homePosCombo
                                width:      parent.width
                                textRole:   "text"
                                model:      _homePositionManager.homePositions

                                onCurrentIndexChanged: {
                                    if (currentIndex != -1) {
                                        var homePos = _homePositionManager.homePositions.get(currentIndex)
                                        _homePositionName = homePos.name
                                        _homePositionCoordinate = homePos.coordinate
                                        editorMap.latitude = _homePositionCoordinate.latitude
                                        editorMap.longitude = _homePositionCoordinate.longitude
                                    }
                                }
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight
                            }

                            QGCLabel {
                                width:      parent.width
                                wrapMode:   Text.WordWrap
                                text:       "To add a new home position, click on the Map to set the position. " +
                                            "Then give it a new name and click Add/Update. " +
                                            "To change the current home position, click on the Map to set the new position. " +
                                            "Then click Add/Update without changing the name."
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight / 3
                            }

                            Item {
                                width:  parent.width
                                height: nameField.height

                                QGCLabel {
                                    anchors.baseline:   nameField.baseline
                                    text:               "Name:"
                                }

                                QGCTextField {
                                    id:             nameField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    text:           _homePositionName
                                }
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight / 3
                            }

                            Item {
                                width:  parent.width
                                height: latitudeField.height

                                QGCLabel {
                                    anchors.baseline:   latitudeField.baseline
                                    text:               "Lat:"
                                }

                                QGCTextField {
                                    id:             latitudeField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    text:           _homePositionCoordinate.latitude
                                }
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight / 3
                            }

                            Item {
                                width:  parent.width
                                height: longitudeField.height

                                QGCLabel {
                                    anchors.baseline:   longitudeField.baseline
                                    text:               "Lon:"
                                }

                                QGCTextField {
                                    id:             longitudeField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    text:           _homePositionCoordinate.longitude
                                }
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight / 3
                            }

                            Item {
                                width:  parent.width
                                height: altitudeField.height

                                QGCLabel {
                                    anchors.baseline:   altitudeField.baseline
                                    text:               "Alt:"
                                }

                                QGCTextField {
                                    id:             altitudeField
                                    anchors.right:  parent.right
                                    width:          _editFieldWidth
                                    text:           _homePositionCoordinate.altitude
                                }
                            }

                            Item {
                                width: 10
                                height: ScreenTools.defaultFontPixelHeight
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text: "Add/Update"

                                    onClicked: {
                                        _homePositionCoordinate = QtPositioning.coordinate(latitudeField.text, longitudeField.text, altitudeField.text)
                                        _homePositionManager.updateHomePosition(nameField.text, _homePositionCoordinate)
                                        homePosCombo.currentIndex = homePosCombo.find(nameField.text)
                                    }
                                }

                                QGCButton {
                                    text: "Delete"

                                    onClicked: {
                                        homePosCombo.currentIndex = -1
                                        _homePositionManager.deleteHomePosition(nameField.text)
                                        homePosCombo.currentIndex = 0
                                        var homePos = _homePositionManager.homePositions.get(0)
                                        _homePositionName = homePos.name
                                        _homePositionCoordinate = homePos.coordinate
                                    }
                                }
                            }
                        } // Column
                    } // Item - Home Position Manager

                    // Help Panel
                    Item {
                        anchors.fill:   parent
                        visible:        !_showHomePositionManager && controller.missionItems.count == 0

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
                            source:             "/qmlimages/MapAddMission.svg"
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
                            id:                 homePositionManagerHelpIcon
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                            anchors.top:        addMissionItemsHelpText.bottom
                            width:              ScreenTools.defaultFontPixelHeight * 3
                            fillMode:           Image.PreserveAspectFit
                            mipmap:             true
                            smooth:             true
                            source:             "/qmlimages/MapHome.svg"
                        }

                        QGCLabel {
                            id:                 homePositionManagerHelpText
                            anchors.leftMargin: ScreenTools.defaultFontPixelHeight
                            anchors.left:       mapTypeHelpIcon.right
                            anchors.right:      parent.right
                            anchors.top:        homePositionManagerHelpIcon.top
                            wrapMode:           Text.WordWrap
                            text:               "<b>Home Position Manager</b><br>" +
                                                "When enabled, allows you to select/add/update home positions. " +
                                                "You can save multiple home position to represent multiple flying areas."
                        }

                        Image {
                            id:                 mapCenterHelpIcon
                            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                            anchors.top:        homePositionManagerHelpText.bottom
                            width:              ScreenTools.defaultFontPixelHeight * 3
                            fillMode:           Image.PreserveAspectFit
                            mipmap:             true
                            smooth:             true
                            source:             "/qmlimages/MapCenter.svg"
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
                            source:             "/qmlimages/MapSync.svg"
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
                            source:             "/qmlimages/MapType.svg"
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
                    } // Item - Help Panel
                } // Item
            } // Rectangle - mission item list
        } // Item - split view container
    } // QGCViewPanel
} // QGCVIew
