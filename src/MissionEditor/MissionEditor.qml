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
import QGroundControl.Mavlink       1.0
import QGroundControl.Controllers   1.0

/// Mission Editor

QGCView {
    viewPanel: panel

    // zOrder comes from the Loader in MainWindow.qml
    z: zOrder

    readonly property int   _decimalPlaces:     7
    readonly property real  _horizontalMargin:  ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _margin:            ScreenTools.defaultFontPixelHeight / 2
    readonly property var   _activeVehicle:     multiVehicleManager.activeVehicle
    readonly property real  _editFieldWidth:    ScreenTools.defaultFontPixelWidth * 16
    readonly property real  _rightPanelWidth:   ScreenTools.defaultFontPixelWidth * 30
    readonly property real  _rightPanelOpacity: 0.8
    readonly property int   _toolButtonCount:   6
    readonly property int   _addMissionItemsButtonAutoOffTimeout:   10000

    property var    _missionItems:              _controller.missionItems

    property var    _homePositionManager:       QGroundControl.homePositionManager
    property string _homePositionName:          _homePositionManager.homePositions.get(0).name

    property var    offlineHomePosition:        _homePositionManager.homePositions.get(0).coordinate
    property var    liveHomePosition:           _controller.liveHomePosition
    property var    liveHomePositionAvailable:  _controller.liveHomePositionAvailable
    property var    homePosition:               offlineHomePosition // live or offline depending on state

    property bool _syncNeeded:                  _controller.missionItems.dirty

    MissionEditorController { id: _controller }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

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

    function updateHomePosition() {
        homePosition = liveHomePositionAvailable ? liveHomePosition : offlineHomePosition
        // Changing the coordinate will set the dirty bit, so we save and reset it
        var dirtyBit = _missionItems.dirty
        _missionItems.get(0).coordinate = homePosition
        _missionItems.dirty = dirtyBit
    }

    Component.onCompleted:              updateHomePosition()
    onOfflineHomePositionChanged:       updateHomePosition()
    onLiveHomePositionAvailableChanged: updateHomePosition()
    onLiveHomePositionChanged:          updateHomePosition()

    Connections {
        target: _controller

        // When the mission items change _missionsItems[0] changes as well so we need to reset it to home
        onMissionItemsChanged: updateHomePosition
    }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Item {
            anchors.fill: parent

            FlightMap {
                id:             editorMap
                anchors.fill:   parent
                mapName:        "MissionEditor"

                Component.onCompleted: {
                    latitude = homePosition.latitude
                    longitude = homePosition.longitude
                }

                readonly property real animationDuration: 500

                Behavior on zoomLevel {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                Behavior on latitude {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                Behavior on longitude {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                        if (homePositionManagerButton.checked) {
                            offlineHomePosition = coordinate
                        } else if (addMissionItemsButton.checked) {
                            var index = _controller.addMissionItem(coordinate)
                            addMissionItemsButtonAutoOffTimer.start()
                            setCurrentItem(index)
                        } else {
                            editorMap.zoomLevel = editorMap.maxZoomLevel - 2
                        }
                    }
                }

                // We use this item to support dragging since dragging a MapQuickItem just doesn't seem to work
                Item {
                    id:         itemEditor
                    x:          missionItemIndicator ? (missionItemIndicator.x + missionItemIndicator.anchorPoint.x - (itemEditor.width / 2)) : 100
                    y:          missionItemIndicator ? (missionItemIndicator.y + missionItemIndicator.anchorPoint.y - (itemEditor.height / 2)) : 100
                    width:      ScreenTools.defaultFontPixelHeight * 7
                    height:     ScreenTools.defaultFontPixelHeight * 7
                    visible:    false
                    z:          editorMap.zOrderMapItems + 1    // Above item icons

                    property var    missionItem
                    property var    missionItemIndicator
                    property real   heading: missionItem ? missionItem.heading : 0

                    Drag.active:    itemDrag.drag.active
                    Drag.hotSpot.x: width  / 2
                    Drag.hotSpot.y: height / 2

                    MissionItemIndexLabel {
                        x:              (itemEditor.width / 2) - (width / 2)
                        y:              (itemEditor.height / 2) - (height / 2)
                        label:          itemEditor.missionItemIndicator ? itemEditor.missionItemIndicator.label : ""
                        isCurrentItem:  true
                    }

                    MouseArea {
                        id:             itemDrag
                        anchors.fill:   parent
                        drag.target:    parent

                        property bool dragActive: drag.active

                        onDragActiveChanged: {
                            if (!drag.active) {
                                var point = Qt.point(itemEditor.x + (itemEditor.width  / 2), itemEditor.y + (itemEditor.height / 2))
                                itemEditor.missionItem.coordinate = editorMap.toCoordinate(point)
                            }
                        }
                    }
                }

                // Add the mission items to the map
                MapItemView {
                    model: _controller.missionItems

                    delegate:
                        MissionItemIndicator {
                            id:             itemIndicator
                            label:          object.sequenceNumber == 0 ? (liveHomePositionAvailable ? "H" : "F") : object.sequenceNumber
                            isCurrentItem:  !homePositionManagerButton.checked && object.isCurrentItem
                            coordinate:     object.coordinate
                            z:              editorMap.zOrderMapItems
                            visible:        object.specifiesCoordinate

                            onClicked: setCurrentItem(object.sequenceNumber)

                            Connections {
                                target: object

                                onIsCurrentItemChanged: {
                                    if (isCurrentItem) {
                                        // Setup our drag item
                                        if (object.sequenceNumber != 0) {
                                            itemEditor.visible = true
                                            itemEditor.missionItem = Qt.binding(function() { return object })
                                            itemEditor.missionItemIndicator = Qt.binding(function() { return itemIndicator })
                                        } else {
                                            itemEditor.visible = false
                                            itemEditor.missionItem = undefined
                                            itemEditor.missionItemIndicator = undefined
                                        }

                                        // Zoom the map and move to the new position
                                        editorMap.zoomLevel = editorMap.maxZoomLevel
                                        editorMap.latitude = object.coordinate.latitude
                                        editorMap.longitude = object.coordinate.longitude
                                    }
                                }
                            }

                            // These are the non-coordinate child mission items attached to this item
                            Row {
                                anchors.top:    parent.top
                                anchors.left:   parent.right

                                Repeater {
                                    model: object.childItems

                                    delegate:
                                        MissionItemIndexLabel {
                                            label:          object.sequenceNumber
                                            isCurrentItem:  !homePositionManagerButton.checked && object.isCurrentItem
                                            z:              2

                                            onClicked: {
                                                setCurrentItem(object.sequenceNumber)
                                                missionItemEditorButton.checked
                                            }

                                        }
                                }
                            }
                        }
                }

                // Add lines between waypoints
                MapItemView {
                    model: _controller.waypointLines

                    delegate:
                        MapPolyline {
                            line.width: 3
                            line.color: qgcPal.mapButtonHighlight
                            z:          editorMap.zOrderMapItems - 1 // Under item indicators

                            path: [
                                { latitude: object.coordinate1.latitude, longitude: object.coordinate1.longitude },
                                { latitude: object.coordinate2.latitude, longitude: object.coordinate2.longitude },
                            ]
                        }
                }

                // Mission Item Editor
                Item {
                    id:             missionItemEditor
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    visible:        !helpButton.checked && !homePositionManagerButton.checked && _missionItems.count > 1
                    opacity:        _rightPanelOpacity
                    z:              editorMap.zOrderTopMost

                    ListView {
                        id:             missionItemSummaryList
                        anchors.fill:   parent
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          _controller.canEdit ? _controller.missionItems : 0

                        property real _maxItemHeight: 0

                        delegate:
                            MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            readOnly:       object.sequenceNumber == 0 && liveHomePositionAvailable

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                var newCurrentItem = object.sequenceNumber - 1
                                _controller.removeMissionItem(object.sequenceNumber)
                                if (_missionItems.count > 1) {
                                    newCurrentItem = Math.min(_missionItems.count - 1, newCurrentItem)
                                    setCurrentItem(newCurrentItem)
                                }
                            }
                        }
                    } // ListView

                    QGCLabel {
                        anchors.fill:   parent
                        visible:        !_controller.canEdit
                        wrapMode:       Text.WordWrap
                        text:           "The set of mission items you have loaded cannot be edited by QGroundControl. " +
                                        "You will only be able to save these to a file, or send them to a vehicle."
                    }
                } // Item - Mission Item editor

                // Home Position Manager
                Rectangle {
                    id:             homePositionManager
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    visible:        homePositionManagerButton.checked
                    color:          qgcPal.window
                    opacity:        _rightPanelOpacity
                    z:              editorMap.zOrderTopMost

                    Column {
                        anchors.margins:    _margin
                        anchors.fill:       parent
                        visible:            !liveHomePositionAvailable

                        QGCLabel {
                            font.pixelSize: ScreenTools.mediumFontPixelSize
                            text:           "Flying Field Manager"
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight
                        }

                        QGCLabel {
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            text:       "This is used to save locations associated with your flying field for use while creating missions with no vehicle connection."
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight
                        }

                        QGCLabel {
                            text:       "Select field to use:"
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
                                    offlineHomePosition = homePos.coordinate
                                    editorMap.latitude = offlineHomePosition.latitude
                                    editorMap.longitude = offlineHomePosition.longitude
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
                            text:       "To add a new flying field, click on the Map to set the position. " +
                                        "Then give it a new name and click Add/Update. " +
                                        "To change the current field position, click on the Map to set the new position. " +
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
                            height: offlineLatitudeField.height

                            QGCLabel {
                                anchors.baseline:   offlineLatitudeField.baseline
                                text:               "Lat:"
                            }

                            QGCTextField {
                                id:             offlineLatitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           offlineHomePosition.latitude
                            }
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight / 3
                        }

                        Item {
                            width:  parent.width
                            height: offlineLongitudeField.height

                            QGCLabel {
                                anchors.baseline:   offlineLongitudeField.baseline
                                text:               "Lon:"
                            }

                            QGCTextField {
                                id:             offlineLongitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           offlineHomePosition.longitude
                            }
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight / 3
                        }

                        Item {
                            width:  parent.width
                            height: offlineAltitudeField.height

                            QGCLabel {
                                anchors.baseline:   offlineAltitudeField.baseline
                                text:               "Alt:"
                            }

                            QGCTextField {
                                id:             offlineAltitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           offlineHomePosition.altitude
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
                                    offlineHomePosition = QtPositioning.coordinate(latitudeField.text, longitudeField.text, altitudeField.text)
                                    _homePositionManager.updateHomePosition(nameField.text, offlineHomePosition)
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
                                    offlineHomePosition = homePos.coordinate
                                }
                            }
                        }
                    } // Column - Offline view

                    Column {
                        anchors.margins:    _margin
                        anchors.fill:       parent
                        visible:            liveHomePositionAvailable

                        QGCLabel {
                            font.pixelSize: ScreenTools.mediumFontPixelSize
                            text:           "Vehicle Home Position"
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight
                        }

                        Item {
                            width:  parent.width
                            height: liveLatitudeField.height

                            QGCLabel {
                                anchors.baseline:   liveLatitudeField.baseline
                                text:               "Lat:"
                            }

                            QGCLabel {
                                id:             liveLatitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           liveHomePosition.latitude
                            }
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight / 3
                        }

                        Item {
                            width:  parent.width
                            height: liveLongitudeField.height

                            QGCLabel {
                                anchors.baseline:   liveLongitudeField.baseline
                                text:               "Lon:"
                            }

                            QGCLabel {
                                id:             liveLongitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           liveHomePosition.longitude
                            }
                        }

                        Item {
                            width: 10
                            height: ScreenTools.defaultFontPixelHeight / 3
                        }

                        Item {
                            width:  parent.width
                            height: liveAltitudeField.height

                            QGCLabel {
                                anchors.baseline:   liveAltitudeField.baseline
                                text:               "Alt:"
                            }

                            QGCLabel {
                                id:             liveAltitudeField
                                anchors.right:  parent.right
                                width:          _editFieldWidth
                                text:           liveHomePosition.altitude
                            }
                        }
                    } // Column - Online view
                } // Item - Home Position Manager

                // Help Panel
                Rectangle {
                    id:             helpPanel
                    anchors.top:    parent.top
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    visible:        !homePositionManagerButton.checked && (_missionItems.count == 1 || helpButton.checked)
                    color:          qgcPal.window
                    opacity:        _rightPanelOpacity
                    z:              editorMap.zOrderTopMost

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
                            text:               "<b>Flying Field Manager</b><br>" +
                                                "When enabled, allows you to select/add/update flying field locations. " +
                                                "You can save multiple flying field locations for use while creating missions while you are not connected to your vehicle."
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
                    } // Item - margin
                } // Item - Help Panel

                RoundButton {
                    id:                     addMissionItemsButton
                    anchors.margins:        _margin
                    anchors.left:           parent.left
                    y:                      (parent.height - (_toolButtonCount * height) - ((_toolButtonCount - 1) * _margin)) / 2
                    buttonImage:            "/qmlimages/MapAddMission.svg"
                    exclusiveGroup:         _dropButtonsExclusiveGroup

                    onCheckedChanged: {
                        if (checked) {
                            addMissionItemsButtonAutoOffTimer.start()
                        } else {
                            addMissionItemsButtonAutoOffTimer.stop()
                        }
                    }

                    Timer {
                        id:         addMissionItemsButtonAutoOffTimer
                        interval:   _addMissionItemsButtonAutoOffTimeout
                        repeat:     false

                        onTriggered: addMissionItemsButton.checked = false
                    }
                }

                RoundButton {
                    id:                 homePositionManagerButton
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        addMissionItemsButton.bottom
                    buttonImage:        "/qmlimages/MapHome.svg"
                    exclusiveGroup:     _dropButtonsExclusiveGroup
                    z:                  editorMap.zOrderWidgets
                }

                DropButton {
                    id:                 centerMapButton
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        homePositionManagerButton.bottom
                    dropDirection:      dropRight
                    buttonImage:        "/qmlimages/MapCenter.svg"
                    viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                    exclusiveGroup:     _dropButtonsExclusiveGroup
                    z:                  editorMap.zOrderWidgets

                    dropDownComponent: Component {
                        Column {
                            QGCLabel { text: "Center map:" }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text: "Home"

                                    onClicked: {
                                        centerMapButton.hideDropDown()
                                        editorMap.center = QtPositioning.coordinate(homePosition.latitude, homePosition.longitude)
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
                            }
                        }
                    }
                }

                DropButton {
                    id:                 syncButton
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        centerMapButton.bottom
                    dropDirection:      dropRight
                    buttonImage:        _syncNeeded ? "/qmlimages/MapSyncChanged.svg" : "/qmlimages/MapSync.svg"
                    viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                    exclusiveGroup:     _dropButtonsExclusiveGroup
                    z:                  editorMap.zOrderWidgets

                    dropDownComponent: Component {
                        Column {
                            id:         columnHolder
                            spacing:    _margin

                            QGCLabel {
                                width:      columnHolder.width
                                wrapMode:   Text.WordWrap
                                text:       _syncNeeded ?
                                                "You have unsaved changed to you mission. You should send to your vehicle, or save to a file:" :
                                                "Sync:"
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text:       "Send to vehicle"
                                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        _controller.setMissionItems()
                                    }
                                }

                                QGCButton {
                                    text:       "Load from vehicle"
                                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        _controller.getMissionItems()
                                    }
                                }
                            }

                            Row {
                                spacing: ScreenTools.defaultFontPixelWidth

                                QGCButton {
                                    text:       "Save to file..."

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        _controller.saveMissionToFile()
                                    }
                                }

                                QGCButton {
                                    text:       "Load from file..."

                                    onClicked: {
                                        syncButton.hideDropDown()
                                        _controller.loadMissionFromFile()
                                    }
                                }
                            }
                        }
                    }
                }

                DropButton {
                    id:                 mapTypeButton
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        syncButton.bottom
                    dropDirection:      dropRight
                    buttonImage:        "/qmlimages/MapType.svg"
                    viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                    exclusiveGroup:     _dropButtonsExclusiveGroup
                    z:                  editorMap.zOrderWidgets

                    dropDownComponent: Component {
                        Column {
                            QGCLabel { text: "Map type:" }

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

                RoundButton {
                    id:                 helpButton
                    anchors.margins:    _margin
                    anchors.left:       parent.left
                    anchors.top:        mapTypeButton.bottom
                    buttonImage:        "/qmlimages/Help.svg"
                    exclusiveGroup:     _dropButtonsExclusiveGroup
                    z:                  editorMap.zOrderWidgets
                }
            } // FlightMap
        } // Item - split view container
    } // QGCViewPanel
} // QGCVIew
