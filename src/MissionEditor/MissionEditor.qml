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

    viewPanel:          panel
    topDialogMargin:    height - mainWindow.availableHeight

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

    property var    _missionItems:          controller.missionItems
    property var    _currentMissionItem

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
        _currentMissionItem = undefined
        for (var i=0; i<_missionItems.count; i++) {
            if (i == index) {
                _currentMissionItem = _missionItems.get(i)
                _currentMissionItem.isCurrentItem = true
            } else {
                _missionItems.get(i).isCurrentItem = false
            }
        }
    }

    property int _moveDialogMissionItemIndex

    Component {
        id: moveDialog

        QGCViewDialog {
            function accept() {
                var toIndex = toCombo.currentIndex

                if (toIndex == 0) {
                    toIndex = 1
                }
                controller.moveMissionItem(_moveDialogMissionItemIndex, toIndex)
                hideDialog()
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           "Move the selected mission item to the be after following mission item:"
                }

                QGCComboBox {
                    id:             toCombo
                    model:          _missionItems.count
                    currentIndex:   _moveDialogMissionItemIndex
                }
            }
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
                    width:          ScreenTools.defaultFontPixelHeight * 2
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    color:          "transparent"
                    visible:        false
                    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

                    property var    missionItem
                    property var    missionItemIndicator
                    property bool   preventCoordinateBindingLoop: false

                    onXChanged: liveDrag()
                    onYChanged: liveDrag()

                    function liveDrag() {
                        if (!itemDragger.preventCoordinateBindingLoop && Drag.active) {
                            var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                            var coordinate = editorMap.toCoordinate(point)
                            coordinate.altitude = itemDragger.missionItem.coordinate.altitude
                            itemDragger.preventCoordinateBindingLoop = true
                            itemDragger.missionItem.coordinate = coordinate
                            itemDragger.preventCoordinateBindingLoop = false
                        }
                    }

                    function clearItem() {
                        itemDragger.visible = false
                        itemDragger.missionItem = undefined
                        itemDragger.missionItemIndicator = undefined
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
                        coordinate:     object.coordinate
                        visible:        object.specifiesCoordinate && (!object.homePosition || object.homePositionValid)
                        z:              QGroundControl.zOrderMapItems
                        missionItem:    object

                        onClicked: setCurrentItem(object.sequenceNumber)

                        function updateItemIndicator()
                        {
                            if (object.isCurrentItem && itemIndicator.visible) {
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

                        Connections {
                            target: object

                            onIsCurrentItemChanged: updateItemIndicator()
                            onCommandChanged:       updateItemIndicator()
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
                    height:         mainWindow.availableHeight
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    visible:        _missionItems.count > 1
                    opacity:        _rightPanelOpacity
                    z:              QGroundControl.zOrderTopMost

                    ListView {
                        anchors.fill:   parent
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          controller.missionItems

                        property real _maxItemHeight: 0

                        delegate:
                            MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            readOnly:       object.sequenceNumber == 0
                            visible:        !readOnly || object.homePositionValid
                            qgcView:        _root

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                itemDragger.clearItem()
                                controller.removeMissionItem(object.sequenceNumber)
                            }

                            onRemoveAll: {
                                itemDragger.clearItem()
                                controller.removeAllMissionItems()
                            }
                        }
                    } // ListView
                } // Item - Mission Item editor

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

                MissionItemStatus {
                    id:                 waypointValuesDisplay
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.bottom:     parent.bottom
                    z:                  QGroundControl.zOrderTopMost
                    currentMissionItem: _currentMissionItem
                    missionItems:       controller.missionItems
                    expandedWidth:      missionItemEditor.x - (ScreenTools.defaultFontPixelWidth * 2)
                    homePositionValid: liveHomePositionAvailable
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
                visible: !ScreenTools.isMobile

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
