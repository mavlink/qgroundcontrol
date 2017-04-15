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
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Mavlink       1.0
import QGroundControl.Controllers   1.0

/// Mission Editor

QGCView {
    id:         _qgcView
    viewPanel:  panel
    z:          QGroundControl.zOrderTopMost

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
    property bool   _lightWidgetBorders:    editorMap.isSatelliteMap
    property bool   _addWaypointOnClick:    false
    property bool   _singleComplexItem:     missionController.complexMissionItemNames.length === 1
    property real   _toolbarHeight:         _qgcView.height - ScreenTools.availableHeight
    property int    _editingLayer:          _layerMission
    property bool   _autoSync:               QGroundControl.settingsManager.appSettings.automaticMissionUpload.rawValue != 0

    /// The controller which should be called for load/save, send to/from vehicle calls
    property var _syncDropDownController: missionController

    readonly property int       _layerMission:              1
    readonly property int       _layerGeoFence:             2
    readonly property int       _layerRallyPoints:          3
    readonly property string    _armedVehicleUploadPrompt:  qsTr("Vehicle is currently armed. Do you want to upload the mission to the vehicle?")

    Component.onCompleted: {
        toolbar.missionController =     Qt.binding(function () { return missionController })
        toolbar.currentMissionItem =    Qt.binding(function () { return _currentMissionItem })
    }

    function addComplexItem(complexItemName) {
        var coordinate = editorMap.center
        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
        insertComplexMissionItem(complexItemName, coordinate, missionController.visualItems.count)
    }

    function insertComplexMissionItem(complexItemName, coordinate, index) {
        var sequenceNumber = missionController.insertComplexMissionItem(complexItemName, coordinate, index)
        setCurrentItem(sequenceNumber)
    }

    property bool _firstMissionLoadComplete:    false
    property bool _firstFenceLoadComplete:      false
    property bool _firstRallyLoadComplete:      false
    property bool _firstLoadComplete:           false

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        editorMap
        usePlannedHomePosition:     true
        mapGeoFenceController:      geoFenceController
        mapMissionController:       missionController
        mapRallyPointController:    rallyPointController
    }

    Connections {
        target: QGroundControl.settingsManager.appSettings.defaultMissionItemAltitude

        onRawValueChanged: {
            if (_visualItems.count > 1) {
                _qgcView.showDialog(applyNewAltitude, qsTr("Apply new alititude"), showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
            }
        }
    }

    Component {
        id: applyNewAltitude

        QGCViewMessage {
            message:    qsTr("You have changed the default altitude for mission items. Would you like to apply that altitude to all the items in the current mission?")

            function accept() {
                hideDialog()
                missionController.applyDefaultMissionAltitude()
            }
        }
    }

    Component {
        id: activeMissionUploadDialogComponent

        QGCViewDialog {

            Column {
                anchors.fill:   parent
                spacing:        ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       qsTr("Your vehicle is currently flying a mission. In order to upload a new or modified mission the current mission will be paused.")
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       qsTr("After the mission is uploaded you can adjust the current waypoint and start the mission.")
                }

                QGCButton {
                    text:       qsTr("Pause and Upload")
                    onClicked: {
                        _activeVehicle.flightMode = _activeVehicle.pauseFlightMode
                        missionController.sendToVehicle()
                        toolbar.showFlyView()
                        hideDialog()
                    }
                }
            }
        }
    }

    MissionController {
        id: missionController

        property var nameFilters: [ qsTr("Mission Files (*.%1)").arg(missionController.fileExtension) , qsTr("All Files (*.*)") ]

        Component.onCompleted: {
            start(true /* editMode */)
            setCurrentItem(0)
        }

        function _denyUpload() {
            if (_activeVehicle && _activeVehicle.armed && _activeVehicle.flightMode === _activeVehicle.missionFlightMode) {
                _qgcView.showDialog(activeMissionUploadDialogComponent, qsTr("Mission Upload"), _qgcView.showDialogDefaultWidth, StandardButton.Cancel)
                return true
            } else {
                return false
            }
        }

        // Users is switching away from Plan View
        function uploadOnSwitch() {
            if (missionController.dirty && _autoSync) {
                if (_denyUpload()) {
                    return false
                } else {
                    sendToVehicle()
                }
            }
            return true
        }

        function upload() {
            if (!_denyUpload()) {
                sendToVehicle()
            }
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select Mission File")
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    missionController.nameFilters
            fileDialog.openForLoad()
        }

        function saveToSelectedFile() {
            fileDialog.title =          qsTr("Save Mission")
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    missionController.nameFilters
            fileDialog.openForSave()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToMissionItems()
        }

        onVisualItemsChanged: itemDragger.clearItem()

        onNewItemsFromVehicle: {
            if (_visualItems && _visualItems.count != 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
            }
            setCurrentItem(0)
        }
    }

    GeoFenceController {
        id: geoFenceController

        property var nameFilters: [ qsTr("GeoFence Files (*.%1)").arg(geoFenceController.fileExtension) , qsTr("All Files (*.*)") ]

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            fileDialog.title =          qsTr("Save GeoFence")
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    geoFenceController.nameFilters
            fileDialog.openForSave()
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select GeoFence File")
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    geoFenceController.nameFilters
            fileDialog.openForLoad()
            ///mapFitFunctions.fitMapViewportToFenceItems()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToFenceItems()
        }
    }

    RallyPointController {
        id: rallyPointController

        property var nameFilters: [ qsTr("Rally Point Files (*.%1)").arg(rallyPointController.fileExtension) , qsTr("All Files (*.*)") ]

        onCurrentRallyPointChanged: {
            if (_editingLayer == _layerRallyPoints && !currentRallyPoint) {
                itemDragger.visible = false
                itemDragger.coordinateItem = undefined
                itemDragger.mapCoordinateIndicator = undefined
            }
        }

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            fileDialog.title =          qsTr("Save Rally Points")
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    rallyPointController.nameFilters
            fileDialog.openForSave()
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select Rally Point File")
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    rallyPointController.nameFilters
            fileDialog.openForLoad()
            //mapFitFunctions.fitMapViewportToRallyItems()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToRallyItems()
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    /// Sets a new current mission item
    ///     @param sequenceNumber - index for new item, -1 to clear current item
    function setCurrentItem(sequenceNumber) {
        if (sequenceNumber !== _currentMissionIndex) {
            _currentMissionItem = undefined
            _currentMissionIndex = -1
            for (var i=0; i<_visualItems.count; i++) {
                var visualItem = _visualItems.get(i)
                if (visualItem.sequenceNumber == sequenceNumber) {
                    _currentMissionItem = visualItem
                    _currentMissionItem.isCurrentItem = true
                    _currentMissionIndex = sequenceNumber
                } else {
                    visualItem.isCurrentItem = false
                }
            }
        }
    }

    /// Inserts a new simple mission item
    ///     @param coordinate Location to insert item
    ///     @param index Insert item at this index
    function insertSimpleMissionItem(coordinate, index) {
        setCurrentItem(-1)
        var sequenceNumber = missionController.insertSimpleMissionItem(coordinate, index)
        setCurrentItem(sequenceNumber)
    }

    property int _moveDialogMissionItemIndex

    QGCFileDialog {
        id:             fileDialog
        qgcView:        _qgcView
        folder:         QGroundControl.settingsManager.appSettings.missionSavePath
        fileExtension:  _syncDropDownController.fileExtension

        onAcceptedForSave: {
            _syncDropDownController.saveToFile(file)
            close()
        }

        onAcceptedForLoad: {
            _syncDropDownController.loadFromFile(file)
            _syncDropDownController.fitViewportToItems()
            _currentMissionItem = _visualItems.get(0)
            close()
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
        anchors.fill:   parent

        FlightMap {
            id:                         editorMap
            anchors.fill:               parent
            mapName:                    "MissionEditor"
            allowGCSLocationCenter:     true
            allowVehicleLocationCenter: true

            // This is the center rectangle of the map which is not obscured by tools
            property rect centerViewport: Qt.rect(_leftToolWidth, _toolbarHeight, editorMap.width - _leftToolWidth - _rightPanelWidth, editorMap.height - _statusHeight - _toolbarHeight)

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

                    var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                    coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                    coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                    coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)

                    switch (_editingLayer) {
                    case _layerMission:
                        if (_addWaypointOnClick) {
                            insertSimpleMissionItem(coordinate, missionController.visualItems.count)
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
                        var coordinate = editorMap.toCoordinate(point, false /* clipToViewPort */)
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
                    map:        editorMap
                    onClicked:  setCurrentItem(sequenceNumber)
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
                    size:           ScreenTools.defaultFontPixelHeight * 3
                    z:              QGroundControl.zOrderMapItems - 1
                }
            }
            GeoFenceMapVisuals {
                map:                    editorMap
                myGeoFenceController:   geoFenceController
                interactive:            _editingLayer == _layerGeoFence
                homePosition:           missionController.plannedHomePosition
                planView:               true
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
                showAlternateIcon:  [ false, false, !_autoSync && _syncDropDownController.dirty, false, false, false ]
                rotateImage:        [ false, false, _syncDropDownController.syncInProgress, false, false, false ]
                animateImage:       [ false, false, !_autoSync && _syncDropDownController.dirty, false, false, false ]
                buttonEnabled:      [ true, true, !_syncDropDownController.syncInProgress, true, true, true ]
                buttonVisible:      [ true, true, true, true, _showZoom, _showZoom ]
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
                        dropPanelComponent: _singleComplexItem ? undefined : patternDropPanel
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
                    case 1:
                        if (_singleComplexItem) {
                            addComplexItem(missionController.complexMissionItemNames[0])
                        }
                        break
                    case 4:
                        editorMap.zoomLevel += 0.5
                        break
                    case 5:
                        editorMap.zoomLevel -= 0.5
                        break
                    }
                }
            }
        } // FlightMap

        // Right pane for mission editing controls
        Rectangle {
            id:                 rightPanel
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            height:             ScreenTools.availableHeight
            width:              _rightPanelWidth
            color:              qgcPal.window
            opacity:            0.2
        }

        Item {
            anchors.fill:   rightPanel

            // Plan Element selector (Mission/Fence/Rally)
            Row {
                id:                 planElementSelectorRow
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _horizontalMargin
                visible:            false // WIP: Temporarily remove - QGroundControl.corePlugin.options.enablePlanViewSelector

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
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                anchors.top:        planElementSelectorRow.visible ? planElementSelectorRow.bottom : planElementSelectorRow.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.bottom:     parent.bottom
                visible:            _editingLayer == _layerMission

                QGCListView {
                    id:             missionItemEditorListView
                    anchors.fill:   parent
                    spacing:        _margin / 2
                    orientation:    ListView.Vertical
                    model:          missionController.visualItems
                    cacheBuffer:    Math.max(height * 2, 0)
                    clip:           true
                    currentIndex:   _currentMissionIndex
                    highlightMoveDuration: 250

                    delegate: MissionItemEditor {
                        map:            editorMap
                        missionItem:    object
                        width:          parent.width
                        readOnly:       false
                        rootQgcView:    _qgcView

                        onClicked:  setCurrentItem(object.sequenceNumber)

                        onRemove: {
                            var removeIndex = index
                            itemDragger.clearItem()
                            missionController.removeMissionItem(removeIndex)
                            if (removeIndex >= missionController.visualItems.count) {
                                removeIndex--
                            }
                            _currentMissionIndex = -1
                            rootQgcView.setCurrentItem(removeIndex)
                        }

                        onInsertWaypoint:       insertSimpleMissionItem(editorMap.center, index)
                        onInsertComplexItem:    insertComplexMissionItem(complexItemName, editorMap.center, index)
                    }
                } // QGCListView
            } // Item - Mission Item editor

            // GeoFence Editor
            Loader {
                anchors.top:        planElementSelectorRow.visible ? planElementSelectorRow.bottom : planElementSelectorRow.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                sourceComponent:    _editingLayer == _layerGeoFence ? geoFenceEditorComponent : undefined

                property real   availableWidth:         _rightPanelWidth
                property real   availableHeight:        ScreenTools.availableHeight
                property var    myGeoFenceController:   geoFenceController
            }

            // Rally Point Editor

            RallyPointEditorHeader {
                id:                 rallyPointHeader
                anchors.top:        planElementSelectorRow.visible ? planElementSelectorRow.bottom : planElementSelectorRow.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                visible:            _editingLayer == _layerRallyPoints
                controller:         rallyPointController
            }

            RallyPointItemEditor {
                id:                 rallyPointEditor
                anchors.top:        planElementSelectorRow.visible ? planElementSelectorRow.bottom : planElementSelectorRow.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                visible:            _editingLayer == _layerRallyPoints && rallyPointController.points.count
                rallyPoint:         rallyPointController.currentRallyPoint
                controller:         rallyPointController
            }
        } // Right panel

        MapScale {
            id:                 mapScale
            anchors.margins:    ScreenTools.defaultFontPixelHeight * (0.66)
            anchors.bottom:     waypointValuesDisplay.visible ? waypointValuesDisplay.top : parent.bottom
            anchors.left:       parent.left
            mapControl:         editorMap
            visible:            !ScreenTools.isTinyScreen
        }

        MissionItemStatus {
            id:                 waypointValuesDisplay
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.left:       parent.left
            anchors.right:      rightPanel.left
            anchors.bottom:     parent.bottom
            missionItems:       missionController.visualItems
            visible:            _editingLayer === _layerMission && !ScreenTools.isShortScreen
        }
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



    Component {
        id: geoFenceEditorComponent

        GeoFenceEditor {
            availableWidth:         _rightPanelWidth
            availableHeight:        ScreenTools.availableHeight
            myGeoFenceController:   geoFenceController
            flightMap:              editorMap
        }
    }

    //- ToolStrip DropPanel Components

    Component {
        id: centerMapDropPanel

        CenterMapDropPanel {
            map:            editorMap
            fitFunctions:   mapFitFunctions
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
                        addComplexItem(modelData)
                        dropPanel.hide()
                    }
                }
            }
        } // Column
    }

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
                                qsTr("You have unsaved changes. You should upload to your vehicle, or save to a file:") :
                                qsTr("Sync:")
            }

            GridLayout {
                id:                 sendSaveGrid
                columns:            2
                anchors.margins:    _margin
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:               qsTr("Upload")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        _syncDropDownController.upload()
                    }
                }

                QGCButton {
                    text:               qsTr("Download")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (_syncDropDownController.dirty) {
                            _qgcView.showDialog(syncLoadFromVehicleOverwrite, columnHolder._overwriteText, _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
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
                            _qgcView.showDialog(syncLoadFromFileOverwrite, columnHolder._overwriteText, _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
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
                        _qgcView.showDialog(removeAllPromptDialog, qsTr("Remove all"), _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                    }
                }
            }

            FactCheckBox {
                text:       qsTr("Automatic upload to vehicle")
                fact:       autoSyncFact
                visible:    autoSyncFact.visible

                property Fact autoSyncFact: QGroundControl.settingsManager.appSettings.automaticMissionUpload
            }
        }
    }
} // QGCVIew
