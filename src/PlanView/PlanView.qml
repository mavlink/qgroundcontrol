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
import QtQuick.Window   2.2

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

    readonly property int   _decimalPlaces:             8
    readonly property real  _horizontalMargin:          ScreenTools.defaultFontPixelWidth  / 2
    readonly property real  _margin:                    ScreenTools.defaultFontPixelHeight * 0.5
    readonly property var   _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    readonly property real  _rightPanelWidth:           Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real  _toolButtonTopMargin:       parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)
    readonly property var   _defaultVehicleCoordinate:  QtPositioning.coordinate(37.803784, -122.462276)
    readonly property bool  _waypointsOnlyMode:         QGroundControl.corePlugin.options.missionWaypointsOnly

    property var    _planMasterController:      masterController
    property var    _missionController:         _planMasterController.missionController
    property var    _geoFenceController:        _planMasterController.geoFenceController
    property var    _rallyPointController:      _planMasterController.rallyPointController
    property var    _visualItems:               _missionController.visualItems
    property bool   _lightWidgetBorders:        editorMap.isSatelliteMap
    property bool   _addWaypointOnClick:        false
    property bool   _addROIOnClick:             false
    property bool   _singleComplexItem:         _missionController.complexMissionItemNames.length === 1
    property real   _toolbarHeight:             _qgcView.height - ScreenTools.availableHeight
    property int    _editingLayer:              _layerMission

    readonly property int       _layerMission:              1
    readonly property int       _layerGeoFence:             2
    readonly property int       _layerRallyPoints:          3
    readonly property string    _armedVehicleUploadPrompt:  qsTr("Vehicle is currently armed. Do you want to upload the mission to the vehicle?")

    Component.onCompleted: {
        toolbar.planMasterController =  Qt.binding(function () { return _planMasterController })
        toolbar.currentMissionItem =    Qt.binding(function () { return _missionController.currentPlanViewItem })
    }

    function addComplexItem(complexItemName) {
        var coordinate = editorMap.center
        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
        insertComplexMissionItem(complexItemName, coordinate, _missionController.visualItems.count)
    }

    function insertComplexMissionItem(complexItemName, coordinate, index) {
        var sequenceNumber = _missionController.insertComplexMissionItem(complexItemName, coordinate, index)
        _missionController.setCurrentPlanViewIndex(sequenceNumber, true)
    }

    property bool _firstMissionLoadComplete:    false
    property bool _firstFenceLoadComplete:      false
    property bool _firstRallyLoadComplete:      false
    property bool _firstLoadComplete:           false

    MapFitFunctions {
        id:                         mapFitFunctions
        map:                        editorMap
        usePlannedHomePosition:     true
        planMasterController:       _planMasterController
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
                _missionController.applyDefaultMissionAltitude()
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
                        _planMasterController.sendToVehicle()
                        hideDialog()
                    }
                }
            }
        }
    }

    Component {
        id: noItemForKML

        QGCViewMessage {
            message:    qsTr("You need at least one item to create a KML.")
        }
    }

    PlanMasterController {
        id: masterController

        Component.onCompleted: {
            start(true /* editMode */)
            _missionController.setCurrentPlanViewIndex(0, true)
        }

        function upload() {
            if (_activeVehicle && _activeVehicle.armed && _activeVehicle.flightMode === _activeVehicle.missionFlightMode) {
                _qgcView.showDialog(activeMissionUploadDialogComponent, qsTr("Plan Upload"), _qgcView.showDialogDefaultWidth, StandardButton.Cancel)
            } else {
                sendToVehicle()
            }
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select Plan File")
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    masterController.loadNameFilters
            fileDialog.openForLoad()
        }

        function saveToSelectedFile() {
            fileDialog.title =          qsTr("Save Plan")
            fileDialog.plan =           true
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    masterController.saveNameFilters
            fileDialog.openForSave()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToMissionItems()
        }

        function saveKmlToSelectedFile() {
            fileDialog.title =          qsTr("Save KML")
            fileDialog.plan =           false
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    masterController.saveKmlFilters
            fileDialog.openForSave()
        }
    }

    Connections {
        target: _missionController

        onNewItemsFromVehicle: {
            if (_visualItems && _visualItems.count != 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
            }
            _missionController.setCurrentPlanViewIndex(0, true)
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    /// Inserts a new simple mission item
    ///     @param coordinate Location to insert item
    ///     @param index Insert item at this index
    function insertSimpleMissionItem(coordinate, index) {
        var sequenceNumber = _missionController.insertSimpleMissionItem(coordinate, index)
        _missionController.setCurrentPlanViewIndex(sequenceNumber, true)
    }

    /// Inserts a new ROI mission item
    ///     @param coordinate Location to insert item
    ///     @param index Insert item at this index
    function insertROIMissionItem(coordinate, index) {
        var sequenceNumber = _missionController.insertROIMissionItem(coordinate, index)
        _missionController.setCurrentPlanViewIndex(sequenceNumber, true)
        _addROIOnClick = false
        toolStrip.uncheckAll()
    }

    property int _moveDialogMissionItemIndex

    QGCFileDialog {
        id:             fileDialog
        qgcView:        _qgcView
        property var plan:           true
        folder:         QGroundControl.settingsManager.appSettings.missionSavePath
        fileExtension:  QGroundControl.settingsManager.appSettings.planFileExtension
        fileExtension2: QGroundControl.settingsManager.appSettings.missionFileExtension

        onAcceptedForSave: {
            plan ? masterController.saveToFile(file) : masterController.saveToKml(file)
            close()
        }

        onAcceptedForLoad: {
            masterController.loadFromFile(file)
            masterController.fitViewportToItems()
            _missionController.setCurrentPlanViewIndex(0, true)
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
                _missionController.moveMissionItem(_moveDialogMissionItemIndex, toIndex)
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
            planView:                   true

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
                            insertSimpleMissionItem(coordinate, _missionController.visualItems.count)
                        } else if (_addROIOnClick) {
                            _addROIOnClick = false
                            insertROIMissionItem(coordinate, _missionController.visualItems.count)
                        }
                        break
                    case _layerRallyPoints:
                        if (_rallyPointController.supported) {
                            _rallyPointController.addPoint(coordinate)
                        }
                        break
                    }
                }
            }

            // Add the mission item visuals to the map
            Repeater {
                model: _editingLayer == _layerMission ? _missionController.visualItems : undefined

                delegate: MissionItemMapVisual {
                    map:        editorMap
                    qgcView:    _qgcView
                    onClicked:  _missionController.setCurrentPlanViewIndex(sequenceNumber, false)
                    visible:    _editingLayer == _layerMission
                }
            }

            // Add lines between waypoints
            MissionLineView {
                model: _editingLayer == _layerMission ? _missionController.waypointLines : undefined
            }

            // Add the vehicles to the map
            MapItemView {
                model: QGroundControl.multiVehicleManager.vehicles
                delegate:
                    VehicleMapItem {
                    vehicle:        object
                    coordinate:     object.coordinate
                    map:            editorMap
                    size:           ScreenTools.defaultFontPixelHeight * 3
                    z:              QGroundControl.zOrderMapItems - 1
                }
            }

            GeoFenceMapVisuals {
                map:                    editorMap
                myGeoFenceController:   _geoFenceController
                interactive:            _editingLayer == _layerGeoFence
                homePosition:           _missionController.plannedHomePosition
                planView:               true
            }

            RallyPointMapVisuals {
                map:                    editorMap
                myRallyPointController: _rallyPointController
                interactive:            _editingLayer == _layerRallyPoints
                planView:               true
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
                showAlternateIcon:  [ false, false, false, masterController.dirty, false, false, false ]
                rotateImage:        [ false, false, false, masterController.syncInProgress, false, false, false ]
                animateImage:       [ false, false, false, masterController.dirty, false, false, false ]
                buttonEnabled:      [ true, true, true, !masterController.syncInProgress, true, true, true ]
                buttonVisible:      [ true, _waypointsOnlyMode, true, true, true, _showZoom, _showZoom ]
                maxHeight:          mapScale.y - toolStrip.y

                property bool _showZoom: !ScreenTools.isMobile

                model: [
                    {
                        name:       "Waypoint",
                        iconSource: "/qmlimages/MapAddMission.svg",
                        toggle:     true
                    },
                    {
                        name:       "ROI",
                        iconSource: "/qmlimages/MapAddMission.svg",
                        toggle:     true
                    },
                    {
                        name:               _singleComplexItem ? _missionController.complexMissionItemNames[0] : "Pattern",
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
                        _addROIOnClick = false
                        break
                    case 1:
                        _addROIOnClick = checked
                        _addWaypointOnClick = false
                        break
                    case 2:
                        if (_singleComplexItem) {
                            addComplexItem(_missionController.complexMissionItemNames[0])
                        }
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
                anchors.topMargin:  Math.round(ScreenTools.defaultFontPixelHeight / 3)
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.right:      parent.right
                spacing:            _horizontalMargin
                visible:            QGroundControl.corePlugin.options.enablePlanViewSelector

                readonly property real _buttonRadius: ScreenTools.defaultFontPixelHeight * 0.75

                ExclusiveGroup {
                    id: planElementSelectorGroup
                    onCurrentChanged: {
                        switch (current) {
                        case planElementMission:
                            _editingLayer = _layerMission
                            break
                        case planElementGeoFence:
                            _editingLayer = _layerGeoFence
                            break
                        case planElementRallyPoints:
                            _editingLayer = _layerRallyPoints
                            break
                        }
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
                    model:          _missionController.visualItems
                    cacheBuffer:    Math.max(height * 2, 0)
                    clip:           true
                    currentIndex:   _missionController.currentPlanViewIndex
                    highlightMoveDuration: 250

                    delegate: MissionItemEditor {
                        map:                editorMap
                        masterController:  _planMasterController
                        missionItem:        object
                        width:              parent.width
                        readOnly:           false
                        rootQgcView:        _qgcView

                        onClicked:  _missionController.setCurrentPlanViewIndex(object.sequenceNumber, false)

                        onRemove: {
                            var removeIndex = index
                            _missionController.removeMissionItem(removeIndex)
                            if (removeIndex >= _missionController.visualItems.count) {
                                removeIndex--
                            }
                            _missionController.setCurrentPlanViewIndex(removeIndex, true)
                        }

                        onInsertWaypoint:       insertSimpleMissionItem(editorMap.center, index)
                        onInsertComplexItem:    insertComplexMissionItem(complexItemName, editorMap.center, index)
                    }
                } // QGCListView
            } // Item - Mission Item editor

            // GeoFence Editor
            GeoFenceEditor {
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight / 2
                anchors.top:            planElementSelectorRow.bottom
                anchors.left:           parent.left
                anchors.right:          parent.right
                availableHeight:        ScreenTools.availableHeight
                myGeoFenceController:   _geoFenceController
                flightMap:              editorMap
                visible:                _editingLayer == _layerGeoFence
            }

            // Rally Point Editor

            RallyPointEditorHeader {
                id:                 rallyPointHeader
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                anchors.top:        planElementSelectorRow.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                visible:            _editingLayer == _layerRallyPoints
                controller:         _rallyPointController
            }

            RallyPointItemEditor {
                id:                 rallyPointEditor
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight / 2
                anchors.top:        rallyPointHeader.bottom
                anchors.left:       parent.left
                anchors.right:      parent.right
                visible:            _editingLayer == _layerRallyPoints && _rallyPointController.points.count
                rallyPoint:         _rallyPointController.currentRallyPoint
                controller:         _rallyPointController
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
            height:             ScreenTools.defaultFontPixelHeight * 7
            maxWidth:           parent.width - rightPanel.width - x
            anchors.bottom:     parent.bottom
            missionItems:       _missionController.visualItems
            visible:            _editingLayer === _layerMission && (ScreenTools.isMobile ? height < Screen.height * 0.25 : true)
        }
    } // QGCViewPanel

    Component {
        id: syncLoadFromVehicleOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading from the Vehicle will lose these changes. Are you sure you want to load from the Vehicle?")
            function accept() {
                hideDialog()
                masterController.loadFromVehicle()
            }
        }
    }

    Component {
        id: syncLoadFromFileOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading from a file will lose these changes. Are you sure you want to load from a file?")
            function accept() {
                hideDialog()
                masterController.loadFromSelectedFile()
            }
        }
    }

    Component {
        id: removeAllPromptDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove all items? ") +
                     (_planMasterController.offline ? "" : qsTr("This will also remove all items from the vehicle."))
            function accept() {
                if (_planMasterController.offline) {
                    masterController.removeAll()
                } else {
                    masterController.removeAllFromVehicle()
                }
                hideDialog()
            }
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
                model: _missionController.complexMissionItemNames

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
                text:       masterController.dirty ?
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
                    enabled:            !masterController.offline && !masterController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        masterController.upload()
                    }
                }

                QGCButton {
                    text:               qsTr("Download")
                    Layout.fillWidth:   true
                    enabled:            !masterController.offline && !masterController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (masterController.dirty) {
                            _qgcView.showDialog(syncLoadFromVehicleOverwrite, columnHolder._overwriteText, _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            masterController.loadFromVehicle()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save To File...")
                    Layout.fillWidth:   true
                    enabled:            !masterController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        masterController.saveToSelectedFile()
                    }
                }

                QGCButton {
                    text:               qsTr("Load From File...")
                    Layout.fillWidth:   true
                    enabled:            !masterController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (masterController.dirty) {
                            _qgcView.showDialog(syncLoadFromFileOverwrite, columnHolder._overwriteText, _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            masterController.loadFromSelectedFile()
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

                QGCButton {
                    text:               qsTr("Save KML...")
                    Layout.fillWidth:   true
                    enabled:            !masterController.syncInProgress
                    onClicked: {
                        // First point do not count
                        if (_visualItems.count < 2) {
                            _qgcView.showDialog(noItemForKML, qsTr("KML"), _qgcView.showDialogDefaultWidth, StandardButton.Cancel)
                            return
                        }
                        dropPanel.hide()
                        masterController.saveKmlToSelectedFile()
                    }
                }
            }
        }
    }
} // QGCVIew
