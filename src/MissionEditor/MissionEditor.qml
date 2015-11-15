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
    id:         _root

    viewPanel:  panel

    // zOrder comes from the Loader in MainWindow.qml
    z: QGroundControl.zOrderTopMost

    readonly property int       _decimalPlaces:     8
    readonly property real      _horizontalMargin:  ScreenTools.defaultFontPixelWidth  / 2
    readonly property real      _margin:            ScreenTools.defaultFontPixelHeight / 2
    readonly property var       _activeVehicle:     multiVehicleManager.activeVehicle
    readonly property real      _editFieldWidth:    ScreenTools.defaultFontPixelWidth * 16
    readonly property real      _rightPanelWidth:   ScreenTools.defaultFontPixelWidth * 30
    readonly property real      _rightPanelOpacity: 0.8
    readonly property int       _toolButtonCount:   6
    readonly property string    _autoSyncKey:       "AutoSync"
    readonly property string    _showHelpKey:       "ShowHelp"
    readonly property int       _addMissionItemsButtonAutoOffTimeout:   10000
    readonly property var       _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)

    property var    _missionItems:  controller.missionItems

    property bool   gpsLock:        _activeVehicle ? _activeVehicle.coordinateValid : false
    property bool   _firstGpsLock:  true

    //property var    _homePositionManager:       QGroundControl.homePositionManager
    //property string _homePositionName:          _homePositionManager.homePositions.get(0).name
    //property var    offlineHomePosition:        _homePositionManager.homePositions.get(0).coordinate

    property var    liveHomePosition:           controller.liveHomePosition
    property var    liveHomePositionAvailable:  controller.liveHomePositionAvailable
    property var    homePosition:               _defaultVehicleCoordinate

    property bool _syncNeeded:                  controller.missionItems.dirty
    property bool _syncInProgress:              _activeVehicle ? _activeVehicle.missionManager.inProgress : false

    property bool _showHelp:                    QGroundControl.flightMapSettings.loadBoolMapSetting(editorMap.mapName, _showHelpKey, true)

    onGpsLockChanged:       updateMapToVehiclePosition()

    Component.onCompleted: {
        helpPanel.source = "MissionEditorHelp.qml"
        updateMapToVehiclePosition()
    }

    function updateMapToVehiclePosition() {
        if (gpsLock && _firstGpsLock) {
            _firstGpsLock = false
            editorMap.latitude = _activeVehicle.latitude
            editorMap.longitude = _activeVehicle.longitude
        }
    }

    function showDistance(missionItem) {
        if (missionItem.distance < 0.0) {
            waypointDistanceDisplay.visible = false
        } else {
            waypointDistanceDisplay.distance = missionItem.distance
            waypointDistanceDisplay.visible = true
        }
    }

    MissionController {
        id:         controller

        Component.onCompleted: {
            start(true /* editMode */)
        }

        /*
        FIXME: autoSync is temporarily disconnected since it's still buggy

        autoSync:   QGroundControl.flightMapSettings.loadMapSetting(editorMap.mapName, _autoSyncKey, true)

        onAutoSyncChanged:      QGroundControl.flightMapSettings.saveMapSetting(editorMap.mapName, _autoSyncKey, autoSync)
*/

        onMissionItemsChanged: itemDragger.clearItem()
    }

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

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        Item {
            anchors.fill: parent

            FlightMap {
                id:             editorMap
                anchors.fill:   parent
                mapName:        "MissionEditor"
                latitude:       mainWindow.tabletPosition.latitude
                longitude:      mainWindow.tabletPosition.longitude

                Component.onCompleted: {
                    console.log("Init coordinate " + mainWindow.tabletPosition.latitude)
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
                        if (false /*homePositionManagerButton.checked*/) {
                            //offlineHomePosition = coordinate
                        } else if (addMissionItemsButton.checked) {
                            var index = controller.addMissionItem(coordinate)
                            addMissionItemsButtonAutoOffTimer.start()
                            setCurrentItem(index)
                        } else {
                            editorMap.zoomLevel = editorMap.maxZoomLevel - 2
                        }
                    }
                }

                // We use this item to support dragging since dragging a MapQuickItem just doesn't seem to work
                Rectangle {
                    id:             itemDragger
                    x:              missionItemIndicator ? (missionItemIndicator.x + missionItemIndicator.anchorPoint.x - (itemDragger.width / 2)) : 100
                    y:              missionItemIndicator ? (missionItemIndicator.y + missionItemIndicator.anchorPoint.y - (itemDragger.height / 2)) : 100
                    width:          _radius * 2
                    height:         _radius * 2
                    //radius:         _radius
                    //border.width:   2
                    //border.color:   "white"
                    color:          "transparent"
                    visible:        false
                    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

                    property var    missionItem
                    property var    missionItemIndicator
                    property real   heading: missionItem ? missionItem.heading : 0

                    readonly property real _radius:         ScreenTools.defaultFontPixelHeight * 4
                    readonly property real _arrowHeight:    ScreenTools.defaultFontPixelHeight

                    function clearItem() {
                        itemDragger.visible = false
                        itemDragger.missionItem = undefined
                        itemDragger.missionItemIndicator = undefined
                    }

                    Image {
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.top:                parent.top
                        height:                     parent._arrowHeight
                        fillMode:                   Image.PreserveAspectFit
                        mipmap:                     true
                        smooth:                     true
                        source:                     "/qmlimages/ArrowHead.svg"
                    }

                    Image {
                        id:                     arrowUp
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right:          parent.right
                        height:                 parent._arrowHeight
                        fillMode:               Image.PreserveAspectFit
                        mipmap:                 true
                        smooth:                 true
                        source:                 "/qmlimages/ArrowHead.svg"
                        transform:              Rotation { origin.x: arrowUp.width / 2; origin.y: arrowUp.height / 2; angle: 90}
                    }

                    Image {
                        id:                         arrowDown
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.bottom:             parent.bottom
                        height:                     parent._arrowHeight
                        fillMode:                   Image.PreserveAspectFit
                        mipmap:                     true
                        smooth:                     true
                        source:                     "/qmlimages/ArrowHead.svg"
                        transform:                  Rotation { origin.x: arrowDown.width / 2; origin.y: arrowDown.height / 2; angle: 180}
                    }

                    Image {
                        id:                     arrowLeft
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left:           parent.left
                        height:                 parent._arrowHeight
                        fillMode:               Image.PreserveAspectFit
                        mipmap:                 true
                        smooth:                 true
                        source:                 "/qmlimages/ArrowHead.svg"
                        transform:              Rotation { origin.x: arrowLeft.width / 2; origin.y: arrowLeft.height / 2; angle: -90}
                    }

                    Rectangle {
                        width:                      _radius * 2
                        height:                     _radius * 2
                        radius:                     _radius
                        anchors.verticalCenter:     parent.verticalCenter
                        anchors.horizontalCenter:   parent.horizontalCenter
                        border.width:               1
                        border.color:               "white"

                        readonly property real _radius: ScreenTools.defaultFontPixelWidth / 4
                    }

                    Drag.active:    itemDrag.drag.active
                    Drag.hotSpot.x: width  / 2
                    Drag.hotSpot.y: height / 2

                    MouseArea {
                        id:             itemDrag
                        anchors.fill:   parent
                        drag.target:    parent
                        drag.minimumX:  0
                        drag.minimumY:  0
                        drag.maximumX:  itemDragger.parent.width - parent.width
                        drag.maximumY:  itemDragger.parent.height - parent.height

                        property bool dragActive: drag.active

                        onDragActiveChanged: {
                            if (!drag.active) {
                                var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                                var coordinate = editorMap.toCoordinate(point)
                                coordinate.altitude = itemDragger.missionItem.coordinate.altitude
                                itemDragger.missionItem.coordinate = coordinate
                                editorMap.latitude = itemDragger.missionItem.coordinate.latitude
                                editorMap.longitude = itemDragger.missionItem.coordinate.longitude
                                _root.showDistance(itemDragger.missionItem)
                            }
                        }
                    }
                }

                // Add the mission items to the map
                MapItemView {
                    model:          controller.missionItems
                    delegate:       delegateComponent
                }

                Component {
                    id: delegateComponent

                    MissionItemIndicator {
                        id:             itemIndicator
                        label:          object.homePosition ? "H" : object.sequenceNumber
                        isCurrentItem:  object.isCurrentItem
                        coordinate:     object.coordinate
                        visible:        object.specifiesCoordinate && (!object.homePosition || object.homePositionValid)
                        z:              QGroundControl.zOrderMapItems

                        onClicked: setCurrentItem(object.sequenceNumber)

                        Connections {
                            target: object

                            onIsCurrentItemChanged: {
                                if (object.isCurrentItem) {
                                    _root.showDistance(object)
                                    if (object.specifiesCoordinate) {
                                        // Setup our drag item
                                        if (object.sequenceNumber != 0) {
                                            itemDragger.visible = true
                                            itemDragger.missionItem = Qt.binding(function() { return object })
                                            itemDragger.missionItemIndicator = Qt.binding(function() { return itemIndicator })
                                        } else {
                                            itemDragger.clearItem()
                                        }

                                        // Move to the new position
                                        editorMap.latitude = object.coordinate.latitude
                                        editorMap.longitude = object.coordinate.longitude
                                    }
                                }
                            }
                        }

                        // These are the non-coordinate child mission items attached to this item
                        Row {
                            anchors.top:    parent.top
                            anchors.left:   parent.right

                            Repeater {
                                model: object.childItems

                                delegate: MissionItemIndexLabel {
                                    label:          object.sequenceNumber
                                    isCurrentItem:  object.isCurrentItem
                                    z:              2

                                    onClicked: setCurrentItem(object.sequenceNumber)
                                }
                            }
                        }
                    }
                }

                // Add lines between waypoints
                MissionLineView {
                    model:          controller.waypointLines
                }

                // Mission Item Editor
                Item {
                    id:             missionItemEditor
                    height:         mainWindow.avaiableHeight
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    visible:        _missionItems.count > 1
                    opacity:        _rightPanelOpacity
                    z:              QGroundControl.zOrderTopMost

                    ListView {
                        id:             missionItemSummaryList
                        anchors.fill:   parent
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          controller.canEdit ? controller.missionItems : 0

                        property real _maxItemHeight: 0

                        delegate:
                            MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            readOnly:       object.sequenceNumber == 0
                            visible:        !readOnly || object.homePositionValid

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                var newCurrentItem = object.sequenceNumber - 1
                                controller.removeMissionItem(object.sequenceNumber)
                                if (_missionItems.count > 1) {
                                    newCurrentItem = Math.min(_missionItems.count - 1, newCurrentItem)
                                    setCurrentItem(newCurrentItem)
                                }
                            }
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

                /*
                  Home Position Manager temporarily disbled till more work is done on it

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
                    z:              QGroundControl.zOrderTopMost

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
                */

                //-- Dismiss Drop Down (if any)
                MouseArea {
                    anchors.fill:   parent
                    enabled:        _dropButtonsExclusiveGroup.current != null
                    onClicked: {
                        if(_dropButtonsExclusiveGroup.current)
                            _dropButtonsExclusiveGroup.current.checked = false
                        _dropButtonsExclusiveGroup.current = null
                    }
                }

                //-- Help Panel
                Loader {
                    id:         helpPanel
                    width:      parent.width  * 0.65
                    height:     parent.height * 0.75
                    z:          QGroundControl.zOrderTopMost
                    anchors.verticalCenter:   parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Item {
                    id:     toolbarSpacer
                    height: mainWindow.tbHeight
                    width:  1
                }

                //-- Vertical Tool Buttons
                Column {
                    id:                         toolColumn
                    anchors.margins:            ScreenTools.defaultFontPixelHeight
                    anchors.left:               parent.left
                    anchors.top:                toolbarSpacer.bottom
                    spacing:                    ScreenTools.defaultFontPixelHeight

                    RoundButton {
                        id:                 addMissionItemsButton
                        buttonImage:        "/qmlimages/MapAddMission.svg"
                        z:                  QGroundControl.zOrderWidgets

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
                        id:                 deleteMissionItemButton
                        buttonImage:        "/qmlimages/TrashDelete.svg"
                        z:                  QGroundControl.zOrderWidgets
                        onClicked: {
                            addMissionItemsButton.checked = false
                            itemDragger.clearItem()
                            controller.deleteCurrentMissionItem()
                            checked = false
                        }
                    }

                    /*
                      Home Position manager temporarily disable
                    RoundButton {
                        id:                 homePositionManagerButton
                        buttonImage:        "/qmlimages/MapHome.svg"
                        //exclusiveGroup:     _dropButtonsExclusiveGroup
                        z:                  QGroundControl.zOrderWidgets
                    }
                    */

                    DropButton {
                        id:                 centerMapButton
                        dropDirection:      dropRight
                        buttonImage:        "/qmlimages/MapCenter.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        z:                  QGroundControl.zOrderWidgets

                        dropDownComponent: Component {
                            Column {
                                QGCLabel { text: "Center map:" }

                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth

                                    QGCButton {
                                        text:       "Home"
                                        enabled:    liveHomePositionAvailable

                                        onClicked: {
                                            centerMapButton.hideDropDown()
                                            editorMap.center = liveHomePosition
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
                        dropDirection:      dropRight
                        buttonImage:        _syncNeeded ? "/qmlimages/MapSyncChanged.svg" : "/qmlimages/MapSync.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        z:                  QGroundControl.zOrderWidgets
                        dropDownComponent:  syncDropDownComponent
                        enabled:            !_syncInProgress
                    }

                    DropButton {
                        id:                 mapTypeButton
                        dropDirection:      dropRight
                        buttonImage:        "/qmlimages/MapType.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        z:                  QGroundControl.zOrderWidgets

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

                    //-- Zoom Map In
                    RoundButton {
                        id:                 mapZoomPlus
                        visible:            !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:        "/qmlimages/ZoomPlus.svg"
                        z:                  QGroundControl.zOrderWidgets
                        onClicked: {
                            if(editorMap)
                                editorMap.zoomLevel += 0.5
                            checked = false
                        }
                    }

                    //-- Zoom Map Out
                    RoundButton {
                        id:                 mapZoomMinus
                        visible:            !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:        "/qmlimages/ZoomMinus.svg"
                        z:                  QGroundControl.zOrderWidgets
                        onClicked: {
                            if(editorMap)
                                editorMap.zoomLevel -= 0.5
                            checked = false
                        }
                    }

                    RoundButton {
                        id:                 helpButton
                        buttonImage:        "/qmlimages/Help.svg"
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        z:                  QGroundControl.zOrderWidgets
                        checked:            _showHelp
                    }
                }

                Rectangle {
                    id:                 waypointDistanceDisplay
                    anchors.margins:    margins
                    anchors.left:       parent.left
                    anchors.bottom:     parent.bottom
                    width:              distanceLabel.width + margins
                    height:             distanceLabel.height + margins
                    radius:             ScreenTools.defaultFontPixelWidth
                    color:              qgcPal.window
                    opacity:            0.80
                    visible:            false

                    property real distance: 0

                    readonly property real margins: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        id:                         distanceLabel
                        anchors.verticalCenter:     parent.verticalCenter
                        anchors.horizontalCenter:   parent.horizonalCenter
                        color:                      qgcPal.text
                        text:                       "Distance: " + Math.round(parent.distance) + " meters"
                    }
                }
            } // FlightMap
        } // Item - split view container
    } // QGCViewPanel

    Component {
        id: syncDropDownComponent

        Column {
            id:         columnHolder
            spacing:    _margin

            QGCLabel {
                width:      columnHolder.width
                wrapMode:   Text.WordWrap
                text:       _syncNeeded && !controller.autoSync ?
                                "You have unsaved changed to you mission. You should send to your vehicle, or save to a file:" :
                                "Sync:"
            }

            Row {
                visible:    true //autoSyncCheckBox.enabled && autoSyncCheckBox.checked
                spacing:    ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       "Send to vehicle"
                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                    onClicked: {
                        syncButton.hideDropDown()
                        controller.sendMissionItems()
                    }
                }

                QGCButton {
                    text:       "Load from vehicle"
                    enabled:    _activeVehicle && !_activeVehicle.missionManager.inProgress

                    onClicked: {
                        syncButton.hideDropDown()
                        controller.getMissionItems()
                    }
                }
            }

            Row {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:       "Save to file..."

                    onClicked: {
                        syncButton.hideDropDown()
                        controller.saveMissionToFile()
                    }
                }

                QGCButton {
                    text:       "Load from file..."

                    onClicked: {
                        syncButton.hideDropDown()
                        controller.loadMissionFromFile()
                    }
                }
            }
/*
        FIXME: autoSync is temporarily disconnected since it's still buggy

            QGCLabel {
                id:         autoSyncDisallowedLabel
                visible:    _activeVehicle && _activeVehicle.armed
                text:       "AutoSync is not allowed whie vehicle is armed"
            }

            QGCCheckBox {
                id:         autoSyncCheckBox
                checked:    controller.autoSync
                text:       "Automatically sync changes with vehicle"
                enabled:    _activeVehicle ? !_activeVehicle.armed : false

                onClicked: controller.autoSync = checked
            }
*/
        }
    }
} // QGCVIew
