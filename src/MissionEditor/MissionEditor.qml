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
    id:         qgcView
    viewPanel:  panel

    // zOrder comes from the Loader in MainWindow.qml
    z: QGroundControl.zOrderTopMost

    readonly property int       _decimalPlaces:         8
    readonly property real      _horizontalMargin:      ScreenTools.defaultFontPixelWidth  / 2
    readonly property real      _margin:                ScreenTools.defaultFontPixelHeight * 0.5
    readonly property var       _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    readonly property real      _rightPanelWidth:       Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real      _rightPanelOpacity:     1
    readonly property int       _toolButtonCount:       6
    readonly property real      _toolButtonTopMargin:   parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)
    readonly property var       _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)

    property var    _visualItems:           missionController.visualItems
    property var    _currentMissionItem
    property int    _currentMissionIndex:   0
    property bool   _firstVehiclePosition:  true
    property var    activeVehiclePosition:  _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property bool   _lightWidgetBorders:    editorMap.isSatelliteMap
    property bool   _addWaypointOnClick:    false

    /// The controller which should be called for load/save, send to/from vehicle calls
    property var _syncDropDownController: missionController

    readonly property int _layerMission:        1
    readonly property int _layerGeoFence:       2
    readonly property int _layerRallyPoints:    3
    property int _editingLayer: _layerMission

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

    property bool _firstMissionLoadComplete:    false
    property bool _firstFenceLoadComplete:      false
    property bool _firstRallyLoadComplete:      false
    property bool _firstLoadComplete:           false

    function checkFirstLoadComplete() {
        if (!_firstLoadComplete && _firstMissionLoadComplete && _firstRallyLoadComplete && _firstFenceLoadComplete) {
            _firstLoadComplete = true
            mapFitFunctions.fitMapViewportToAllItems()
        }
    }

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        editorMap
        mapFitViewport:             editorMap.centerViewport
        usePlannedHomePosition:     true
        mapGeoFenceController:      geoFenceController
        mapMissionController:       missionController
        mapRallyPointController:    rallyPointController
    }

    MissionController {
        id: missionController

        Component.onCompleted: {
            start(true /* editMode */)
            setCurrentItem(0)
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Mission File"), qgcView.showDialogDefaultWidth, StandardButton.Cancel)
            } else {
                missionController.loadFromFilePicker()
                mapFitFunctions.fitMapViewportToMissionItems()
                _currentMissionItem = _visualItems.get(0)
            }
        }

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Mission File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                missionController.saveToFilePicker()
            }
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToMissionItems()
        }

        onVisualItemsChanged: {
            itemDragger.clearItem()
        }

        onNewItemsFromVehicle: {
            if (_visualItems && _visualItems.count != 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
            }
            setCurrentItem(0)
            _firstMissionLoadComplete = true
            checkFirstLoadComplete()
        }
    }

    GeoFenceController {
        id: geoFenceController

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Fence File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                geoFenceController.saveToFilePicker()
            }
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Fence File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
            } else {
                geoFenceController.loadFromFilePicker()
                mapFitFunctions.fitMapViewportToFenceItems()
            }
        }

        function validateBreachReturn() {
            if (geoFenceController.polygon.path.length > 0) {
                if (!geoFenceController.polygon.containsCoordinate(geoFenceController.breachReturnPoint)) {
                    geoFenceController.breachReturnPoint = geoFenceController.polygon.center()
                }
                if (!geoFenceController.polygon.containsCoordinate(geoFenceController.breachReturnPoint)) {
                    geoFenceController.breachReturnPoint = geoFenceController.polygon.path[0]
                }
            }
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToFenceItems()
        }

        onLoadComplete: {
            _firstFenceLoadComplete = true
            switch (_syncDropDownController) {
            case geoFenceController:
                mapFitFunctions.fitMapViewportToFenceItems()
                break
            case missionController:
                checkFirstLoadComplete()
                break
            }
        }
    }

    RallyPointController {
        id: rallyPointController

        onCurrentRallyPointChanged: {
            if (_editingLayer == _layerRallyPoints && !currentRallyPoint) {
                itemDragger.visible = false
                itemDragger.coordinateItem = undefined
                itemDragger.mapCoordinateIndicator = undefined
            }
        }

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Rally Point File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                rallyPointController.saveToFilePicker()
            }
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Rally Point File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
            } else {
                rallyPointController.loadFromFilePicker()
                mapFitFunctions.fitMapViewportToRallyItems()
            }
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToRallyItems()
        }

        onLoadComplete: {
            _firstRallyLoadComplete = true
            switch (_syncDropDownController) {
            case rallyPointController:
                mapFitFunctions.fitMapViewportToRallyItems()
                break
            case missionController:
                checkFirstLoadComplete()
                break
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    function setCurrentItem(sequenceNumber) {
        if (sequenceNumber !== _currentMissionIndex) {
            _currentMissionItem = undefined
            _currentMissionIndex = -1
            for (var i=0; i<_visualItems.count; i++) {
                var visualItem = _visualItems.get(i)
                if (visualItem.sequenceNumber == sequenceNumber) {
                    _currentMissionItem = visualItem
                    _currentMissionItem.isCurrentItem = true
                    _currentMissionIndex = i
                } else {
                    visualItem.isCurrentItem = false
                }
            }
        }
    }

    property int _moveDialogMissionItemIndex

    Component {
        id: mobileFilePicker

        QGCMobileFileOpenDialog {
            fileExtension: _syncDropDownController.fileExtension
            onFilenameReturned: {
                _syncDropDownController.loadFromFile(filename)
                _syncDropDownController.fitViewportToItems()
            }
        }
    }

    Component {
        id: mobileFileSaver

        QGCMobileFileSaveDialog {
            fileExtension:      _syncDropDownController.fileExtension
            onFilenameReturned: _syncDropDownController.saveToFile(filename)
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
                missionController.moveMissionItem(_moveDialogMissionItemIndex, toIndex)
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
                height:         qgcView.height
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
                anchors.right:  parent.right
                mapName:        "MissionEditor"

                // This is the center rectangle of the map which is not obscured by tools
                property rect centerViewport: Qt.rect(_leftToolWidth, _toolbarHeight, editorMap.width - _leftToolWidth - _rightPanelWidth, editorMap.height - _statusHeight - _toolbarHeight)

                property real _toolbarHeight:   qgcView.height - ScreenTools.availableHeight
                property real _leftToolWidth:   toolStrip.x + toolStrip.width
                property real _statusHeight:    waypointValuesDisplay.visible ? editorMap.height - waypointValuesDisplay.y : 0

                readonly property real animationDuration: 500

                // Initial map position duplicates Fly view position
                Component.onCompleted: editorMap.center = QGroundControl.flightMapPosition

                Behavior on zoomLevel {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                QGCMapPalette { id: mapPal; lightColors: editorMap.isSatelliteMap }

                MouseArea {
                    //-- It's a whole lot faster to just fill parent and deal with top offset below
                    //   than computing the coordinate offset.
                    anchors.fill: parent
                    onClicked: {
                        //-- Don't pay attention to items beneath the toolbar.
                        var topLimit = parent.height - ScreenTools.availableHeight
                        if(mouse.y < topLimit) {
                            return
                        }

                        var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)

                        switch (_editingLayer) {
                        case _layerMission:
                            if (_addWaypointOnClick) {
                                var sequenceNumber = missionController.insertSimpleMissionItem(coordinate, missionController.visualItems.count)
                                setCurrentItem(sequenceNumber)
                            }
                            break
                        case _layerGeoFence:
                            if (geoFenceController.breachReturnEnabled) {
                                geoFenceController.breachReturnPoint = coordinate
                                geoFenceController.validateBreachReturn()
                            }
                            break
                        case _layerRallyPoints:
                            if (rallyPointController.rallyPointsSupported) {
                                rallyPointController.addPoint(coordinate)
                            }
                            break
                        }
                    }
                }

                // We use this item to support dragging since dragging a MapQuickItem just doesn't seem to work
                Rectangle {
                    id:             itemDragger
                    x:              mapCoordinateIndicator ? (mapCoordinateIndicator.x + mapCoordinateIndicator.anchorPoint.x - (itemDragger.width / 2)) : 100
                    y:              mapCoordinateIndicator ? (mapCoordinateIndicator.y + mapCoordinateIndicator.anchorPoint.y - (itemDragger.height / 2)) : 100
                    width:          ScreenTools.defaultFontPixelHeight * 3
                    height:         ScreenTools.defaultFontPixelHeight * 3
                    color:          "transparent"
                    visible:        false
                    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

                    property var    coordinateItem
                    property var    mapCoordinateIndicator
                    property bool   preventCoordinateBindingLoop: false

                    onXChanged: liveDrag()
                    onYChanged: liveDrag()

                    function liveDrag() {
                        if (!itemDragger.preventCoordinateBindingLoop && Drag.active) {
                            var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                            var coordinate = editorMap.toCoordinate(point)
                            coordinate.altitude = itemDragger.coordinateItem.coordinate.altitude
                            itemDragger.preventCoordinateBindingLoop = true
                            itemDragger.coordinateItem.coordinate = coordinate
                            itemDragger.preventCoordinateBindingLoop = false
                        }
                    }

                    function clearItem() {
                        itemDragger.visible = false
                        itemDragger.coordinateItem = undefined
                        itemDragger.mapCoordinateIndicator = undefined
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

                // Add the mission item visuals to the map
                Repeater {
                    model: missionController.visualItems

                    delegate: MissionItemMapVisual {
                        map: editorMap
                    }
                }

                // Add lines between waypoints
                MissionLineView {
                    model:      _editingLayer == _layerMission ? missionController.waypointLines : undefined
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

                // Plan Element selector (Mission/Fence/Rally)
                Row {
                    id:                 planElementSelectorRow
                    anchors.topMargin:  parent.height - ScreenTools.availableHeight + _margin
                    anchors.top:        parent.top
                    anchors.leftMargin: parent.width - _rightPanelWidth
                    anchors.left:       parent.left
                    spacing:            _horizontalMargin
                    visible:            QGroundControl.corePlugin.options.enablePlanViewSelector

                    readonly property real _buttonRadius: ScreenTools.defaultFontPixelHeight * 0.75

                    ExclusiveGroup {
                        id: planElementSelectorGroup
                        onCurrentChanged: {
                            switch (current) {
                            case planElementMission:
                                _editingLayer = _layerMission
                                _syncDropDownController = missionController
                                break
                            case planElementGeoFence:
                                _editingLayer = _layerGeoFence
                                _syncDropDownController = geoFenceController
                                break
                            case planElementRallyPoints:
                                _editingLayer = _layerRallyPoints
                                _syncDropDownController = rallyPointController
                                break
                            }
                            _syncDropDownController.fitViewportToItems()
                        }
                    }

                    QGCRadioButton {
                        id:             planElementMission
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Mission")
                        checked:        true
                        color:          mapPal.text
                        textStyle:      Text.Outline
                        textStyleColor: mapPal.textOutline
                    }

                    Item { height: 1; width: 1 }

                    QGCRadioButton {
                        id:             planElementGeoFence
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Fence")
                        color:          mapPal.text
                        textStyle:      Text.Outline
                        textStyleColor: mapPal.textOutline
                    }

                    Item { height: 1; width: 1 }

                    QGCRadioButton {
                        id:             planElementRallyPoints
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Rally")
                        color:          mapPal.text
                        textStyle:      Text.Outline
                        textStyleColor: mapPal.textOutline
                    }
                } // Row - Plan Element Selector

                // Mission Item Editor
                Item {
                    id:                 missionItemEditor
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.visible ? planElementSelectorRow.bottom : planElementSelectorRow.top
                    anchors.bottom:     parent.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerMission

                    MouseArea {
                        // This MouseArea prevents the Map below it from getting Mouse events. Without this
                        // things like mousewheel will scroll the Flickable and then scroll the map as well.
                        anchors.fill:       missionItemEditorListView
                        onWheel:            wheel.accepted = true
                    }

                    QGCListView {
                        id:             missionItemEditorListView
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.top:    parent.top
                        height:         parent.height
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          missionController.visualItems
                        cacheBuffer:    height * 2
                        clip:           true
                        currentIndex:   _currentMissionIndex
                        highlightMoveDuration: 250

                        delegate: MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            readOnly:       false

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                var removeIndex = index
                                itemDragger.clearItem()
                                missionController.removeMissionItem(removeIndex)
                                if (removeIndex >= missionController.visualItems.count) {
                                    removeIndex--
                                }
                                setCurrentItem(removeIndex)
                            }

                            onInsert: {
                                var sequenceNumber = missionController.insertSimpleMissionItem(editorMap.center, index)
                                setCurrentItem(sequenceNumber)
                            }

                            onMoveHomeToMapCenter: _visualItems.get(0).coordinate = editorMap.center
                        }
                    } // QGCListView
                } // Item - Mission Item editor

                // GeoFence Editor
                Loader {
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.bottom
                    anchors.right:      parent.right
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    source:             _editingLayer == _layerGeoFence ? "qrc:/qml/GeoFenceEditor.qml" : ""

                    property real availableWidth:   _rightPanelWidth
                    property real availableHeight:  ScreenTools.availableHeight
                }

                // GeoFence polygon
                MapPolygon {
                    border.color:   "#80FF0000"
                    border.width:   3
                    path:           geoFenceController.polygon.path
                    z:              QGroundControl.zOrderMapItems
                    visible:        geoFenceController.polygonEnabled
                }

                // GeoFence circle
                MapCircle {
                    border.color:   "#80FF0000"
                    border.width:   3
                    center:         missionController.plannedHomePosition
                    radius:         geoFenceController.circleRadius
                    z:              QGroundControl.zOrderMapItems
                    visible:        geoFenceController.circleEnabled
                }

                // GeoFence breach return point
                MapQuickItem {
                    anchorPoint.x:  sourceItem.anchorPointX
                    anchorPoint.y:  sourceItem.anchorPointY
                    coordinate:     geoFenceController.breachReturnPoint
                    visible:        geoFenceController.breachReturnEnabled
                    sourceItem:     MissionItemIndexLabel { label: "F" }
                    z:              QGroundControl.zOrderMapItems
                }

                // Rally Point Editor

                RallyPointEditorHeader {
                    id:                 rallyPointHeader
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerRallyPoints
                    controller:         rallyPointController
                }

                RallyPointItemEditor {
                    id:                 rallyPointEditor
                    anchors.topMargin:  _margin
                    anchors.top:        rallyPointHeader.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerRallyPoints && rallyPointController.points.count
                    rallyPoint:         rallyPointController.currentRallyPoint
                    controller:         rallyPointController
                }

                // Rally points on map

                MapItemView {
                    model: rallyPointController.points

                    delegate: MapQuickItem {
                        id:             itemIndicator
                        anchorPoint.x:  sourceItem.anchorPointX
                        anchorPoint.y:  sourceItem.anchorPointY
                        coordinate:     object.coordinate
                        z:              QGroundControl.zOrderMapItems

                        sourceItem: MissionItemIndexLabel {
                            id:         itemIndexLabel
                            label:      qsTr("R", "rally point map item label")
                            checked:    _editingLayer == _layerRallyPoints ? object == rallyPointController.currentRallyPoint : false

                            onClicked: rallyPointController.currentRallyPoint = object

                            onCheckedChanged: {
                                if (checked) {
                                    // Setup our drag item
                                    itemDragger.visible = true
                                    itemDragger.coordinateItem = Qt.binding(function() { return object })
                                    itemDragger.mapCoordinateIndicator = Qt.binding(function() { return itemIndicator })
                                }
                            }
                        }
                    }
                }

                ToolStrip {
                    id:                 toolStrip
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.topMargin:  _toolButtonTopMargin
                    anchors.top:        parent.top
                    color:              qgcPal.window
                    title:              qsTr("Plan")
                    z:                  QGroundControl.zOrderWidgets
                    showAlternateIcon:  [ false, false, _syncDropDownController.dirty, false, false, false, false ]
                    rotateImage:        [ false, false, _syncDropDownController.syncInProgress, false, false, false, false ]
                    buttonEnabled:      [ true, true, !_syncDropDownController.syncInProgress, true, true, true, true ]
                    buttonVisible:      [ true, true, true, true, true, _showZoom, _showZoom ]
                    maxHeight:          mapScale.y - toolStrip.y

                    property bool _showZoom: !ScreenTools.isMobile

                    model: [
                        {
                            name:       "Waypoint",
                            iconSource: "/qmlimages/MapAddMission.svg",
                            toggle:     true
                        },
                        {
                            name:               "Pattern",
                            iconSource:         "/qmlimages/MapDrawShape.svg",
                            dropPanelComponent: patternDropPanel
                        },
                        {
                            name:                   "Sync",
                            iconSource:             "/qmlimages/MapSync.svg",
                            alternateIconSource:    "/qmlimages/MapSyncChanged.svg",
                            dropPanelComponent:     syncDropPanel
                        },
                        {
                            name:               "Center",
                            iconSource:         "/qmlimages/MapCenter.svg",
                            dropPanelComponent: centerMapDropPanel
                        },
                        {
                            name:               "Map",
                            iconSource:         "/qmlimages/MapType.svg",
                            dropPanelComponent: mapTypeDropPanel
                        },
                        {
                            name:               "In",
                            iconSource:         "/qmlimages/ZoomPlus.svg"
                        },
                        {
                            name:               "Out",
                            iconSource:         "/qmlimages/ZoomMinus.svg"
                        }
                    ]

                    onClicked: {
                        switch (index) {
                        case 0:
                            _addWaypointOnClick = checked
                            break
                        case 5:
                            editorMap.zoomLevel += 0.5
                            break
                        case 6:
                            editorMap.zoomLevel -= 0.5
                            break
                        case 5:
                            editorMap.zoomLevel += 0.5
                            break
                        case 6:
                            editorMap.zoomLevel -= 0.5
                            break
                        }
                    }
                }

                MapScale {
                    id:                 mapScale
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * (0.66)
                    anchors.bottom:     waypointValuesDisplay.visible ? waypointValuesDisplay.top : parent.bottom
                    anchors.left:       parent.left
                    mapControl:         editorMap
                    visible:            !ScreenTools.isTinyScreen
                }

                MissionItemStatus {
                    id:                     waypointValuesDisplay
                    anchors.margins:        ScreenTools.defaultFontPixelWidth
                    anchors.left:           parent.left
                    anchors.bottom:         parent.bottom
                    z:                      QGroundControl.zOrderTopMost
                    currentMissionItem:     _currentMissionItem
                    missionItems:           missionController.visualItems
                    expandedWidth:          missionItemEditor.x - (ScreenTools.defaultFontPixelWidth * 2)
                    missionDistance:        missionController.missionDistance
                    missionTime:            missionController.missionTime
                    missionMaxTelemetry:    missionController.missionMaxTelemetry
                    visible:                _editingLayer == _layerMission && !ScreenTools.isShortScreen
                }
            } // FlightMap
        } // Item - split view container
    } // QGCViewPanel

    Component {
        id: syncLoadFromVehicleOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading from the Vehicle will lose these changes. Are you sure you want to load from the Vehicle?")
            function accept() {
                hideDialog()
                _syncDropDownController.loadFromVehicle()
            }
        }
    }

    Component {
        id: syncLoadFromFileOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading a from a file will lose these changes. Are you sure you want to load from a file?")
            function accept() {
                hideDialog()
                _syncDropDownController.loadFromSelectedFile()
            }
        }
    }

    Component {
        id: removeAllPromptDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove all items?")
            function accept() {
                itemDragger.clearItem()
                _syncDropDownController.removeAll()
                hideDialog()
            }
        }
    }

    //- ToolStrip DropPanel Components

    Component {
        id: syncDropPanel

        Column {
            id:         columnHolder
            spacing:    _margin

            property string _overwriteText: (_editingLayer == _layerMission) ? qsTr("Mission overwrite") : ((_editingLayer == _layerGeoFence) ? qsTr("GeoFence overwrite") : qsTr("Rally Points overwrite"))

            QGCLabel {
                width:      sendSaveGrid.width
                wrapMode:   Text.WordWrap
                text:       _syncDropDownController.dirty ?
                                qsTr("You have unsaved changes. You should send to your vehicle, or save to a file:") :
                                qsTr("Sync:")
            }

            GridLayout {
                id:                 sendSaveGrid
                columns:            2
                anchors.margins:    _margin
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:               qsTr("Send To Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        _syncDropDownController.sendToVehicle()
                    }
                }

                QGCButton {
                    text:               qsTr("Load From Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (_syncDropDownController.dirty) {
                            qgcView.showDialog(syncLoadFromVehicleOverwrite, columnHolder._overwriteText, qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _syncDropDownController.loadFromVehicle()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save To File...")
                    Layout.fillWidth:   true
                    enabled:            !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        _syncDropDownController.saveToSelectedFile()
                    }
                }

                QGCButton {
                    text:               qsTr("Load From File...")
                    Layout.fillWidth:   true
                    enabled:            !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (_syncDropDownController.dirty) {
                            qgcView.showDialog(syncLoadFromFileOverwrite, columnHolder._overwriteText, qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _syncDropDownController.loadFromSelectedFile()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Remove All")
                    Layout.fillWidth:   true
                    onClicked:  {
                        dropPanel.hide()
                        qgcView.showDialog(removeAllPromptDialog, qsTr("Remove all"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                    }
                }
            }
        }
    }

    Component {
        id: centerMapDropPanel

        CenterMapDropPanel {
            map:            editorMap
            fitFunctions:   mapFitFunctions
        }
    }

    Component {
        id: mapTypeDropPanel

        Column {
            spacing: _margin

            QGCLabel { text: qsTr("Map type:") }
            Row {
                spacing: ScreenTools.defaultFontPixelWidth
                Repeater {
                    model: QGroundControl.flightMapSettings.mapTypes

                    QGCButton {
                        checkable:      true
                        checked:        QGroundControl.flightMapSettings.mapType === text
                        text:           modelData
                        exclusiveGroup: _mapTypeButtonsExclusiveGroup

                        onClicked: {
                            QGroundControl.flightMapSettings.mapType = text
                            dropPanel.hide()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: patternDropPanel

        ColumnLayout {
            spacing:    ScreenTools.defaultFontPixelWidth * 0.5

            QGCLabel { text: qsTr("Create complex pattern:") }

            Repeater {
                model: missionController.complexMissionItemNames

                QGCButton {
                    text:               modelData
                    Layout.fillWidth:   true

                    onClicked: {
                        var coordinate = editorMap.center
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                        var sequenceNumber = missionController.insertComplexMissionItem(modelData, coordinate, missionController.visualItems.count)
                        setCurrentItem(sequenceNumber)
                        dropPanel.hide()
                    }
                }
            }
        } // Column
    }
} // QGCVIew
