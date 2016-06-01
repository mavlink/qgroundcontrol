/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Layouts  1.2

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

    property bool syncNeeded: controller.visualItems.dirty // Unsaved changes, visible to parent container

    viewPanel:          panel

    // zOrder comes from the Loader in MainWindow.qml
    z: QGroundControl.zOrderTopMost

    readonly property int       _decimalPlaces:     8
    readonly property real      _horizontalMargin:  ScreenTools.defaultFontPixelWidth  / 2
    readonly property real      _margin:            ScreenTools.defaultFontPixelHeight * 0.5
    readonly property var       _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    readonly property real      _editFieldWidth:    ScreenTools.defaultFontPixelWidth * 16
    readonly property real      _rightPanelWidth:   Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real      _rightPanelOpacity: 0.8
    readonly property int       _toolButtonCount:   6
    readonly property string    _autoSyncKey:       "AutoSync"
    readonly property int       _addMissionItemsButtonAutoOffTimeout:   10000
    readonly property var       _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)

    property var    _visualItems:          controller.visualItems
    property var    _currentMissionItem
    property bool   _firstVehiclePosition:  true
    property var    activeVehiclePosition:  _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property bool   _lightWidgetBorders:    editorMap.isSatelliteMap

    onActiveVehiclePositionChanged: updateMapToVehiclePosition()

    Connections {
        target: QGroundControl.multiVehicleManager

        onActiveVehicleChanged: {
            // When the active vehicle changes we need to allow the first vehicle position to move the map again
            _firstVehiclePosition = true
            updateMapToVehiclePosition()
        }
    }

    function updateMapToVehiclePosition() {
        if (_activeVehicle && _activeVehicle.coordinateValid && _activeVehicle.coordinate.isValid && _firstVehiclePosition) {
            _firstVehiclePosition = false
            editorMap.center = _activeVehicle.coordinate
        }
    }

    function loadFromVehicle() {
        controller.getMissionItems()
    }

    function loadFromFile() {
        if (ScreenTools.isMobile) {
            _root.showDialog(mobileFilePicker, qsTr("Select Mission File"), _root.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
        } else {
            controller.loadMissionFromFilePicker()
            fitViewportToMissionItems()
        }
    }

    function saveToFile() {
        if (ScreenTools.isMobile) {
            _root.showDialog(mobileFileSaver, qsTr("Save Mission File"), _root.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
        } else {
            controller.saveMissionToFilePicker()
        }
    }

    function normalizeLat(lat) {
        // Normalize latitude to range: 0 to 180, S to N
        return lat + 90.0
    }

    function normalizeLon(lon) {
        // Normalize longitude to range: 0 to 360, W to E
        return lon  + 180.0
    }

    /// Fix the map viewport to the current mission items.
    function fitViewportToMissionItems() {
        if (_visualItems.count == 1) {
            editorMap.center = _visualItems.get(0).coordinate
        } else {
            var missionItem = _visualItems.get(0)
            var north = normalizeLat(missionItem.coordinate.latitude)
            var south = north
            var east = normalizeLon(missionItem.coordinate.longitude)
            var west = east

            for (var i=1; i<_visualItems.count; i++) {
                missionItem = _visualItems.get(i)

                if (missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate) {
                    var lat = normalizeLat(missionItem.coordinate.latitude)
                    var lon = normalizeLon(missionItem.coordinate.longitude)

                    north = Math.max(north, lat)
                    south = Math.min(south, lat)
                    east = Math.max(east, lon)
                    west = Math.min(west, lon)
                }
            }
            editorMap.visibleRegion = QtPositioning.rectangle(QtPositioning.coordinate(north - 90.0, west - 180.0), QtPositioning.coordinate(south - 90.0, east - 180.0))
        }
    }

    MissionController {
        id:         controller

        Component.onCompleted: {
            start(true /* editMode */)
            setCurrentItem(0)
        }

        /*
        FIXME: autoSync is temporarily disconnected since it's still buggy

        autoSync:   QGroundControl.flightMapSettings.loadMapSetting(editorMap.mapName, _autoSyncKey, true)

        onAutoSyncChanged:      QGroundControl.flightMapSettings.saveMapSetting(editorMap.mapName, _autoSyncKey, autoSync)
*/

        onVisualItemsChanged: itemDragger.clearItem()
        onNewItemsFromVehicle: fitViewportToMissionItems()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    function setCurrentItem(sequenceNumber) {
        _currentMissionItem = undefined
        for (var i=0; i<_visualItems.count; i++) {
            var visualItem = _visualItems.get(i)
            if (visualItem.sequenceNumber == sequenceNumber) {
                _currentMissionItem = visualItem
                _currentMissionItem.isCurrentItem = true
            } else {
                visualItem.isCurrentItem = false
            }
        }
    }

    property int _moveDialogMissionItemIndex

    Component {
        id: mobileFilePicker

        QGCMobileFileDialog {
            openDialog:     true
            fileExtension:  QGroundControl.missionFileExtension

            onFilenameReturned: {
                controller.loadMissionFromFile(filename)
                fitViewportToMissionItems()
            }
        }
    }

    Component {
        id: mobileFileSaver

        QGCMobileFileDialog {
            openDialog:     false
            fileExtension:  QGroundControl.missionFileExtension

            onFilenameReturned: {
                controller.saveMissionToFile(filename)
            }
        }
    }

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
                    text:           qsTr("Move the selected mission item to the be after following mission item:")
                }

                QGCComboBox {
                    id:             toCombo
                    model:          _visualItems.count
                    currentIndex:   _moveDialogMissionItemIndex
                }
            }
        }
    }

    QGCViewPanel {
        id:             panel
        height:         ScreenTools.availableHeight
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right

        Item {
            anchors.fill: parent

            FlightMap {
                id:             editorMap
                height:         _root.height
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
                anchors.right:  parent.right
                mapName:        "MissionEditor"

                signal mapClicked(var coordinate)

                readonly property real animationDuration: 500

                // Initial map position duplicates Fly view position
                Component.onCompleted: editorMap.center = QGroundControl.flightMapPosition

                Behavior on zoomLevel {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                MouseArea {
                    //-- It's a whole lot faster to just fill parent and deal with top offset below
                    //   than computing the coordinate offset.
                    anchors.fill: parent
                    onClicked: {
                        //-- Don't pay attention to items beneath the toolbar.
                        var topLimit = parent.height - ScreenTools.availableHeight
                        if(mouse.y >= topLimit) {
                            var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                            coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                            coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                            coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                            if (addMissionItemsButton.checked) {
                                var sequenceNumber = controller.insertSimpleMissionItem(coordinate, controller.visualItems.count)
                                setCurrentItem(sequenceNumber)
                                editorListView.positionViewAtIndex(editorListView.count - 1, ListView.Contain)
                            } else {
                                editorMap.mapClicked(coordinate)
                            }
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

                // Add the complex mission item polygon to the map
                MapItemView {
                    model: controller.complexVisualItems
                    delegate: MapPolygon {
                        color:      'green'
                        path:       object.polygonPath
                        opacity:    0.5
                    }
                }

                // Add the complex mission item grid to the map
                MapItemView {
                    model: controller.complexVisualItems

                    delegate: MapPolyline {
                        line.color: "white"
                        path:       object.gridPoints
                    }
                }

                // Add the complex mission item exit coordinates
                MapItemView {
                    model:      controller.complexVisualItems
                    delegate:   exitCoordinateComponent
                }

                Component {
                    id: exitCoordinateComponent

                    MissionItemIndicator {
                        coordinate:     object.exitCoordinate
                        z:              QGroundControl.zOrderMapItems
                        missionItem:    object
                        sequenceNumber: object.lastSequenceNumber
                        visible:        object.specifiesCoordinate
                    }
                }

                // Add the simple mission items to the map
                MapItemView {
                    model:      controller.visualItems
                    delegate:   missionItemComponent
                }

                Component {
                    id: missionItemComponent

                    MissionItemIndicator {
                        id:             itemIndicator
                        coordinate:     object.coordinate
                        visible:        object.specifiesCoordinate
                        z:              QGroundControl.zOrderMapItems
                        missionItem:    object
                        sequenceNumber: object.sequenceNumber

                        //-- If you don't want to allow selecting items beneath the
                        //   toolbar, the code below has to check and see if mouse.y
                        //   is greater than (map.height - ScreenTools.availableHeight)
                        onClicked: setCurrentItem(object.sequenceNumber)

                        function updateItemIndicator() {
                            if (object.isCurrentItem && itemIndicator.visible && object.specifiesCoordinate && object.isSimpleItem) {
                                // Setup our drag item
                                itemDragger.visible = true
                                itemDragger.missionItem = Qt.binding(function() { return object })
                                itemDragger.missionItemIndicator = Qt.binding(function() { return itemIndicator })
                            }
                        }

                        Connections {
                            target: object

                            onIsCurrentItemChanged:         updateItemIndicator()
                            onSpecifiesCoordinateChanged:   updateItemIndicator()
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

                // Add the vehicles to the map
                MapItemView {
                    model: QGroundControl.multiVehicleManager.vehicles
                    delegate:
                        VehicleMapItem {
                                vehicle:        object
                                coordinate:     object.coordinate
                                isSatellite:    editorMap.isSatelliteMap
                                size:           ScreenTools.defaultFontPixelHeight * 5
                                z:              QGroundControl.zOrderMapItems - 1
                        }
                }

                // Mission Item Editor
                Item {
                    id:             missionItemEditor
                    height:         ScreenTools.availableHeight
                    anchors.bottom: parent.bottom
                    anchors.right:  parent.right
                    width:          _rightPanelWidth
                    opacity:        _rightPanelOpacity
                    z:              QGroundControl.zOrderTopMost

                    MouseArea {
                         // This MouseArea prevents the Map below it from getting Mouse events. Without this
                         // things like mousewheel will scroll the Flickable and then scroll the map as well.
                         anchors.fill:       editorListView
                         onWheel:            wheel.accepted = true
                     }

                    ListView {
                        id:             editorListView
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.top:    parent.top
                        height:         parent.height
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          controller.visualItems
                        cacheBuffer:    height * 2
                        clip:           true

                        delegate: MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            qgcView:        _root
                            readOnly:       false

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                itemDragger.clearItem()
                                controller.removeMissionItem(index)
                            }

                            onInsert: {
                                var sequenceNumber = controller.insertSimpleMissionItem(editorMap.center, i)
                                setCurrentItem(sequenceNumber)
                            }

                            onMoveHomeToMapCenter: controller.visualItems.get(0).coordinate = editorMap.center

                            Connections {
                                target: object

                                onIsCurrentItemChanged: {
                                    if (object.isCurrentItem) {
                                        editorListView.positionViewAtIndex(index, ListView.Contain)
                                    }
                                }
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

                //-- Vertical Tool Buttons
                Column {
                    id:                 toolColumn
                    anchors.topMargin:  parent.height - ScreenTools.availableHeight + ScreenTools.defaultFontPixelHeight
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight
                    z:                  QGroundControl.zOrderWidgets

                    RoundButton {
                        id:             addMissionItemsButton
                        buttonImage:    "/qmlimages/MapAddMission.svg"
                        lightBorders:   _lightWidgetBorders
                    }

                    RoundButton {
                        id:             addShapeButton
                        buttonImage:    "/qmlimages/MapDrawShape.svg"
                        visible:        QGroundControl.experimentalSurvey
                        lightBorders:   _lightWidgetBorders

                        onClicked: {
                            var coordinate = editorMap.center
                            coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                            coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                            coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                            var sequenceNumber = controller.insertComplexMissionItem(coordinate, controller.visualItems.count)
                            setCurrentItem(sequenceNumber)
                            checked = false
                            addMissionItemsButton.checked = false
                        }
                    }

                    DropButton {
                        id:                 syncButton
                        dropDirection:      dropRight
                        buttonImage:        syncNeeded ? "/qmlimages/MapSyncChanged.svg" : "/qmlimages/MapSync.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        dropDownComponent:  syncDropDownComponent
                        enabled:            !controller.syncInProgress
                        rotateImage:        controller.syncInProgress
                        lightBorders:       _lightWidgetBorders
                    }

                    DropButton {
                        id:                 centerMapButton
                        dropDirection:      dropRight
                        buttonImage:        "/qmlimages/MapCenter.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        lightBorders:       _lightWidgetBorders

                        dropDownComponent: Component {
                            Column {
                                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                                QGCLabel { text: qsTr("Center map:") }
                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    QGCButton {
                                        text: qsTr("Home")
                                        width:  ScreenTools.defaultFontPixelWidth * 10
                                        onClicked: {
                                            centerMapButton.hideDropDown()
                                            editorMap.center = controller.visualItems.get(0).coordinate
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("Mission")
                                        width:  ScreenTools.defaultFontPixelWidth * 10
                                        onClicked: {
                                            centerMapButton.hideDropDown()
                                            fitViewportToMissionItems()
                                        }
                                    }
                                    QGCButton {
                                        text:       qsTr("Vehicle")
                                        width:      ScreenTools.defaultFontPixelWidth * 10
                                        enabled:    activeVehicle && activeVehicle.latitude != 0 && activeVehicle.longitude != 0
                                        property var activeVehicle: _activeVehicle
                                        onClicked: {
                                            centerMapButton.hideDropDown()
                                            editorMap.center = activeVehicle.coordinate
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
                        lightBorders:       _lightWidgetBorders

                        dropDownComponent: Component {
                            Column {
                                spacing: _margin
                                QGCLabel { text: qsTr("Map type:") }
                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    Repeater {
                                        model: QGroundControl.flightMapSettings.mapTypes
                                        QGCButton {
                                            checkable:      true
                                            checked:        editorMap.mapType === text
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
                        id:             mapZoomPlus
                        visible:        !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:    "/qmlimages/ZoomPlus.svg"
                        lightBorders:   _lightWidgetBorders

                        onClicked: {
                            if(editorMap)
                                editorMap.zoomLevel += 0.5
                            checked = false
                        }
                    }

                    //-- Zoom Map Out
                    RoundButton {
                        id:             mapZoomMinus
                        visible:        !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:    "/qmlimages/ZoomMinus.svg"
                        lightBorders:   _lightWidgetBorders
                        onClicked: {
                            if(editorMap)
                                editorMap.zoomLevel -= 0.5
                            checked = false
                        }
                    }
                }

                MissionItemStatus {
                    id:                 waypointValuesDisplay
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.bottom:     parent.bottom
                    z:                  QGroundControl.zOrderTopMost
                    currentMissionItem: _currentMissionItem
                    missionItems:       controller.visualItems
                    expandedWidth:      missionItemEditor.x - (ScreenTools.defaultFontPixelWidth * 2)
                    visible:            !ScreenTools.isShortScreen
                }
            } // FlightMap
        } // Item - split view container
    } // QGCViewPanel

    Component {
        id: syncLoadFromVehicleOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent mission changes. Loading the mission from the Vehicle will lose these changes. Are you sure you want to load the mission from the Vehicle?")
            function accept() {
                hideDialog()
                loadFromVehicle()
            }
        }
    }

    Component {
        id: syncLoadFromFileOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent mission changes. Loading a mission from a file will lose these changes. Are you sure you want to load a mission from a file?")
            function accept() {
                hideDialog()
                loadFromFile()
            }
        }
    }

    Component {
        id: removeAllPromptDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to delete all mission items?")
            function accept() {
                itemDragger.clearItem()
                controller.removeAllMissionItems()
                hideDialog()
            }
        }
    }

    Component {
        id: syncDropDownComponent
        Column {
            id:         columnHolder
            spacing:    _margin
            QGCLabel {
                width:      sendSaveGrid.width
                wrapMode:   Text.WordWrap
                text:       syncNeeded && !controller.autoSync ?
                                qsTr("You have unsaved changed to you mission. You should send to your vehicle, or save to a file:") :
                                qsTr("Sync:")
            }
            GridLayout {
                id:                 sendSaveGrid
                columns:            2
                anchors.margins:    _margin
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth
                visible:            true //autoSyncCheckBox.enabled && autoSyncCheckBox.checked
                QGCButton {
                    text:               qsTr("Send To Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !controller.syncInProgress
                    onClicked: {
                        syncButton.hideDropDown()
                        controller.sendMissionItems()
                    }
                }
                QGCButton {
                    text:               qsTr("Load From Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !controller.syncInProgress
                    onClicked: {
                        syncButton.hideDropDown()
                        if (syncNeeded) {
                            _root.showDialog(syncLoadFromVehicleOverwrite, qsTr("Mission overwrite"), _root.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            loadFromVehicle()
                        }
                    }
                }
                QGCButton {
                    text:               qsTr("Save To File...")
                    Layout.fillWidth:   true
                    enabled:            !controller.syncInProgress
                    onClicked: {
                        syncButton.hideDropDown()
                        saveToFile()
                    }
                }
                QGCButton {
                    text:               qsTr("Load From File...")
                    Layout.fillWidth:   true
                    enabled:            !controller.syncInProgress
                    onClicked: {
                        syncButton.hideDropDown()
                        if (syncNeeded) {
                            _root.showDialog(syncLoadFromFileOverwrite, qsTr("Mission overwrite"), _root.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            loadFromFile()
                        }
                    }
                }
                QGCButton {
                    text:               qsTr("Remove All")
                    Layout.fillWidth:   true
                    onClicked:  {
                        syncButton.hideDropDown()
                        _root.showDialog(removeAllPromptDialog, qsTr("Delete all"), _root.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
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
