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

import QGroundControl                   1.0
import QGroundControl.FlightMap         1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactSystem        1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0
import QGroundControl.Controllers       1.0
import QGroundControl.ShapeFileHelper   1.0
import QGroundControl.Airspace          1.0
import QGroundControl.Airmap            1.0

/// Mission Editor

QGCView {
    id:         _qgcView
    viewPanel:  panel
    z:          QGroundControl.zOrderTopMost

    property bool planControlColapsed: false

    ///< This property is used to determine dirty state for prompting on QGC shutdown
    readonly property bool dirty: _planMasterController.dirty

    readonly property int   _decimalPlaces:             8
    readonly property real  _horizontalMargin:          ScreenTools.defaultFontPixelWidth  * 0.5
    readonly property real  _margin:                    ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real  _radius:                    ScreenTools.defaultFontPixelWidth  * 0.5
    readonly property var   _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    readonly property real  _rightPanelWidth:           Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real  _toolButtonTopMargin:       parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)
    readonly property var   _defaultVehicleCoordinate:  QtPositioning.coordinate(37.803784, -122.462276)
    readonly property bool  _waypointsOnlyMode:         QGroundControl.corePlugin.options.missionWaypointsOnly

    property bool   _airspaceEnabled:                    QGroundControl.airmapSupported ? (QGroundControl.settingsManager.airMapSettings.enableAirMap.rawValue && QGroundControl.airspaceManager.connected): false
    property var    _planMasterController:              masterController
    property var    _missionController:                 _planMasterController.missionController
    property var    _geoFenceController:                _planMasterController.geoFenceController
    property var    _rallyPointController:              _planMasterController.rallyPointController
    property var    _visualItems:                       _missionController.visualItems
    property bool   _lightWidgetBorders:                editorMap.isSatelliteMap
    property bool   _addWaypointOnClick:                false
    property bool   _addROIOnClick:                     false
    property bool   _singleComplexItem:                 _missionController.complexMissionItemNames.length === 1
    property real   _toolbarHeight:                     _qgcView.height - ScreenTools.availableHeight
    property int    _editingLayer:                      _layerMission
    property int    _toolStripBottom:                   toolStrip.height + toolStrip.y
    property var    _appSettings:                       QGroundControl.settingsManager.appSettings

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
        coordinate.latitude  = coordinate.latitude.toFixed(_decimalPlaces)
        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
        coordinate.altitude  = coordinate.altitude.toFixed(_decimalPlaces)
        insertComplexMissionItem(complexItemName, coordinate, _missionController.visualItems.count)
    }

    function insertComplexMissionItem(complexItemName, coordinate, index) {
        var sequenceNumber = _missionController.insertComplexMissionItem(complexItemName, coordinate, index)
        _missionController.setCurrentPlanViewIndex(sequenceNumber, true)
    }

    function insertComplexMissionItemFromKMLOrSHP(complexItemName, file, index) {
        var sequenceNumber = _missionController.insertComplexMissionItemFromKMLOrSHP(complexItemName, file, index)
        _missionController.setCurrentPlanViewIndex(sequenceNumber, true)
    }

    function updateAirspace(reset) {
        if(_airspaceEnabled) {
            var coordinateNW = editorMap.toCoordinate(Qt.point(0,0), false /* clipToViewPort */)
            var coordinateSE = editorMap.toCoordinate(Qt.point(width,height), false /* clipToViewPort */)
            if(coordinateNW.isValid && coordinateSE.isValid) {
                QGroundControl.airspaceManager.setROI(coordinateNW, coordinateSE, true /*planView*/, reset)
            }
        }
    }

    property bool _firstMissionLoadComplete:    false
    property bool _firstFenceLoadComplete:      false
    property bool _firstRallyLoadComplete:      false
    property bool _firstLoadComplete:           false

    MapFitFunctions {
        id:                         mapFitFunctions  // The name for this id cannot be changed without breaking references outside of this code. Beware!
        map:                        editorMap
        usePlannedHomePosition:     true
        planMasterController:       _planMasterController
    }

    on_AirspaceEnabledChanged: {
        if(QGroundControl.airmapSupported) {
            if(_airspaceEnabled) {
                planControlColapsed = QGroundControl.airspaceManager.airspaceVisible
                updateAirspace(true)
            } else {
                planControlColapsed = false
            }
        } else {
            planControlColapsed = false
        }
    }

    Connections {
        target: _appSettings.defaultMissionItemAltitude

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

    Connections {
        target: QGroundControl.airspaceManager
        onAirspaceVisibleChanged: {
            planControlColapsed = QGroundControl.airspaceManager.airspaceVisible
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
            start(false /* flyView */)
            _missionController.setCurrentPlanViewIndex(0, true)
        }

        function waitingOnDataMessage() {
            _qgcView.showMessage(qsTr("Unable to Save/Upload"), qsTr("Plan is waiting on terrain data from server for correct altitude values."), StandardButton.Ok)
        }

        function upload() {
            if (!readyForSaveSend()) {
                waitingOnDataMessage()
                return
            }
            if (_activeVehicle && _activeVehicle.armed && _activeVehicle.flightMode === _activeVehicle.missionFlightMode) {
                _qgcView.showDialog(activeMissionUploadDialogComponent, qsTr("Plan Upload"), _qgcView.showDialogDefaultWidth, StandardButton.Cancel)
            } else {
                sendToVehicle()
            }
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select Plan File")
            fileDialog.planFiles =      true
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    masterController.loadNameFilters
            fileDialog.fileExtension =  _appSettings.planFileExtension
            fileDialog.fileExtension2 = _appSettings.missionFileExtension
            fileDialog.openForLoad()
        }

        function saveToSelectedFile() {
            if (!readyForSaveSend()) {
                waitingOnDataMessage()
                return
            }
            fileDialog.title =          qsTr("Save Plan")
            fileDialog.planFiles =      true
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    masterController.saveNameFilters
            fileDialog.fileExtension =  _appSettings.planFileExtension
            fileDialog.fileExtension2 = _appSettings.missionFileExtension
            fileDialog.openForSave()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToMissionItems()
        }

        function loadShapeFromSelectedFile() {
            fileDialog.title =          qsTr("Load Shape")
            fileDialog.planFiles =      false
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    ShapeFileHelper.fileDialogKMLOrSHPFilters
            fileDialog.fileExtension =  _appSettings.kmlFileExtension
            fileDialog.fileExtension2 = _appSettings.shpFileExtension
            fileDialog.openForLoad()
        }

        function saveKmlToSelectedFile() {
            if (!readyForSaveSend()) {
                waitingOnDataMessage()
                return
            }
            fileDialog.title =          qsTr("Save KML")
            fileDialog.planFiles =      false
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    ShapeFileHelper.fileDialogKMLFilters
            fileDialog.fileExtension =  _appSettings.kmlFileExtension
            fileDialog.fileExtension2 = ""
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
        folder:         _appSettings.missionSavePath

        property bool planFiles: true    ///< true: working with plan files, false: working with kml file

        onAcceptedForSave: {
            if (planFiles) {
                masterController.saveToFile(file)
            } else {
                masterController.saveToKml(file)
            }
            close()
        }

        onAcceptedForLoad: {
            if (planFiles) {
                masterController.loadFromFile(file)
                masterController.fitViewportToItems()
                _missionController.setCurrentPlanViewIndex(0, true)
            } else {
                var retList = ShapeFileHelper.determineShapeType(file)
                if (retList[0] == ShapeFileHelper.Error) {
                    _qgcView.showMessage("Error", retList[1], StandardButton.Ok)
                } else if (retList[0] == ShapeFileHelper.Polygon) {
                     var editVehicle = _activeVehicle ? _activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
                    if (editVehicle.fixedWing) {
                        insertComplexMissionItemFromKMLOrSHP(_missionController.surveyComplexItemName, file, -1)
                    } else {
                        polygonSelectPatternFile = file
                        _qgcView.showDialog(patternPolygonSelectDialog, fileDialog.title, _qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                    }
                } else if (retList[0] == ShapeFileHelper.Polyline) {
                    insertComplexMissionItemFromKMLOrSHP(_missionController.corridorScanComplexItemName, file, -1)
                }
            }
            close()
        }
    }

    property string polygonSelectPatternFile
    Component {
        id: patternPolygonSelectDialog

        QGCViewDialog {
            function accept() {
                var complexItemName
                if (surveyRadio.checked) {
                    complexItemName = _missionController.surveyComplexItemName
                } else {
                    complexItemName = _missionController.structureScanComplexItemName
                }
                insertComplexMissionItemFromKMLOrSHP(complexItemName, polygonSelectPatternFile, -1)
                hideDialog()
            }

            ExclusiveGroup {
                id: radioGroup
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           qsTr("Create which pattern type?")
                }

                QGCRadioButton {
                    id:             surveyRadio
                    text:           qsTr("Survey")
                    checked:        true
                    exclusiveGroup: radioGroup
                }

                QGCRadioButton {
                    text:           qsTr("Structure Scan")
                    exclusiveGroup: radioGroup
                }
            }
        }
    }

    Component {
        id: moveDialog

        QGCViewDialog {
            function accept() {
                var toIndex = toCombo.currentIndex

                if (toIndex === 0) {
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
            qgcView:                    _qgcView

            // This is the center rectangle of the map which is not obscured by tools
            property rect centerViewport:   Qt.rect(_leftToolWidth, _toolbarHeight, editorMap.width - _leftToolWidth - _rightPanelWidth, editorMap.height - _statusHeight - _toolbarHeight)

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

            onZoomLevelChanged: updateAirspace(false)
            onCenterChanged:    updateAirspace(false)

            MouseArea {
                //-- It's a whole lot faster to just fill parent and deal with top offset below
                //   than computing the coordinate offset.
                anchors.fill: parent
                onClicked: {
                    // Take focus to close any previous editing
                    editorMap.focus = true

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

            // Airspace overlap support
            MapItemView {
                model:              _airspaceEnabled && QGroundControl.airspaceManager.airspaceVisible ? QGroundControl.airspaceManager.airspaces.circles : []
                delegate: MapCircle {
                    center:         object.center
                    radius:         object.radius
                    color:          object.color
                    border.color:   object.lineColor
                    border.width:   object.lineWidth
                }
            }

            MapItemView {
                model:              _airspaceEnabled && QGroundControl.airspaceManager.airspaceVisible ? QGroundControl.airspaceManager.airspaces.polygons : []
                delegate: MapPolygon {
                    path:           object.polygon
                    color:          object.color
                    border.color:   object.lineColor
                    border.width:   object.lineWidth
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
                showAlternateIcon:  [ masterController.dirty, false, false, false, false, false, false ]
                rotateImage:        [ masterController.syncInProgress, false, false, false, false, false, false ]
                animateImage:       [ masterController.dirty, false, false, false, false, false, false ]
                buttonEnabled:      [ !masterController.syncInProgress, true, true, true, true, true, true ]
                buttonVisible:      [ true, true, _waypointsOnlyMode, true, true, _showZoom, _showZoom ]
                maxHeight:          mapScale.y - toolStrip.y

                property bool _showZoom: !ScreenTools.isMobile

                model: [
                    {
                        name:                   qsTr("File"),
                        iconSource:             "/qmlimages/MapSync.svg",
                        alternateIconSource:    "/qmlimages/MapSyncChanged.svg",
                        dropPanelComponent:     syncDropPanel
                    },
                    {
                        name:                   qsTr("Waypoint"),
                        iconSource:             "/qmlimages/MapAddMission.svg",
                        toggle:                 true
                    },
                    {
                        name:                   qsTr("ROI"),
                        iconSource:             "/qmlimages/MapAddMission.svg",
                        toggle:                 true
                    },
                    {
                        name:               _singleComplexItem ? _missionController.complexMissionItemNames[0] : qsTr("Pattern"),
                        iconSource:         "/qmlimages/MapDrawShape.svg",
                        dropPanelComponent: _singleComplexItem ? undefined : patternDropPanel
                    },
                    {
                        name:               qsTr("Center"),
                        iconSource:         "/qmlimages/MapCenter.svg",
                        dropPanelComponent: centerMapDropPanel
                    },
                    {
                        name:               qsTr("In"),
                        iconSource:         "/qmlimages/ZoomPlus.svg"
                    },
                    {
                        name:               qsTr("Out"),
                        iconSource:         "/qmlimages/ZoomMinus.svg"
                    }
                ]

                onClicked: {
                    switch (index) {
                    case 1:
                        _addWaypointOnClick = checked
                        _addROIOnClick = false
                        break
                    case 2:
                        _addROIOnClick = checked
                        _addWaypointOnClick = false
                        break
                    case 3:
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
        }
        //-----------------------------------------------------------
        // Right pane for mission editing controls
        Rectangle {
            id:                 rightPanel
            height:             ScreenTools.availableHeight
            width:              _rightPanelWidth
            color:              qgcPal.window
            opacity:            planExpanded.visible ? 0.2 : 0
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            anchors.rightMargin: ScreenTools.defaultFontPixelWidth
        }
        //-------------------------------------------------------
        // Right Panel Controls
        Item {
            anchors.fill:           rightPanel
            Column {
                id:                 rightControls
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.25
                //-------------------------------------------------------
                // Airmap Airspace Control
                AirspaceControl {
                    id:             airspaceControl
                    width:          parent.width
                    visible:        _airspaceEnabled
                    planView:       true
                    showColapse:    true
                }
                //-------------------------------------------------------
                // Mission Controls (Colapsed)
                Rectangle {
                    width:      parent.width
                    height:     planControlColapsed ? colapsedRow.height + ScreenTools.defaultFontPixelHeight : 0
                    color:      qgcPal.missionItemEditor
                    radius:     _radius
                    visible:    planControlColapsed && _airspaceEnabled
                    Row {
                        id:                     colapsedRow
                        spacing:                ScreenTools.defaultFontPixelWidth
                        anchors.left:           parent.left
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                        anchors.verticalCenter: parent.verticalCenter
                        QGCColoredImage {
                            width:              height
                            height:             ScreenTools.defaultFontPixelWidth * 2.5
                            sourceSize.height:  height
                            source:             "qrc:/res/waypoint.svg"
                            color:              qgcPal.text
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            text:               qsTr("Plan")
                            color:              qgcPal.text
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    QGCColoredImage {
                        width:                  height
                        height:                 ScreenTools.defaultFontPixelWidth * 2.5
                        sourceSize.height:      height
                        source:                 QGroundControl.airmapSupported ? "qrc:/airmap/expand.svg" : ""
                        color:                  "white"
                        visible:                QGroundControl.airmapSupported
                        anchors.right:          parent.right
                        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    MouseArea {
                        anchors.fill:   parent
                        enabled:        QGroundControl.airmapSupported
                        onClicked: {
                            QGroundControl.airspaceManager.airspaceVisible = false
                        }
                    }
                }
                //-------------------------------------------------------
                // Mission Controls (Expanded)
                Rectangle {
                    id:         planExpanded
                    width:      parent.width
                    height:     (!planControlColapsed || !_airspaceEnabled) ? expandedCol.height + ScreenTools.defaultFontPixelHeight : 0
                    color:      qgcPal.missionItemEditor
                    radius:     _radius
                    visible:    !planControlColapsed || !_airspaceEnabled
                    Item {
                        height:             expandedCol.height
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        Column {
                            id:                     expandedCol
                            spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            //-- Header
                            Row {
                                id:                     expandedRow
                                spacing:                ScreenTools.defaultFontPixelWidth
                                anchors.left:           parent.left
                                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                                readonly property real _buttonRadius: ScreenTools.defaultFontPixelHeight * 0.75
                                QGCLabel {
                                    text:           qsTr("Plan")
                                    color:          qgcPal.text
                                    visible:        !QGroundControl.corePlugin.options.enablePlanViewSelector
                                    anchors.verticalCenter: parent.verticalCenter
                                }
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
                                    visible:        QGroundControl.corePlugin.options.enablePlanViewSelector
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QGCRadioButton {
                                    id:             planElementGeoFence
                                    exclusiveGroup: planElementSelectorGroup
                                    text:           qsTr("Fence")
                                    visible:        QGroundControl.corePlugin.options.enablePlanViewSelector
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                                QGCRadioButton {
                                    id:             planElementRallyPoints
                                    exclusiveGroup: planElementSelectorGroup
                                    text:           qsTr("Rally")
                                    visible:        QGroundControl.corePlugin.options.enablePlanViewSelector
                                    anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                }
            }
            //-------------------------------------------------------
            // Mission Item Editor
            Item {
                id:                     missionItemEditor
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.top:            rightControls.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
                anchors.bottom:         parent.bottom
                anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
                visible:                _editingLayer == _layerMission && !planControlColapsed
                QGCListView {
                    id:             missionItemEditorListView
                    anchors.fill:   parent
                    spacing:        ScreenTools.defaultFontPixelHeight / 4
                    orientation:    ListView.Vertical
                    model:          _missionController.visualItems
                    cacheBuffer:    Math.max(height * 2, 0)
                    clip:           true
                    currentIndex:   _missionController.currentPlanViewIndex
                    highlightMoveDuration: 250
                    visible:        _editingLayer == _layerMission && !planControlColapsed
                    //-- List Elements
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
                }
            }
            // GeoFence Editor
            GeoFenceEditor {
                anchors.top:            rightControls.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
                anchors.bottom:         parent.bottom
                anchors.left:           parent.left
                anchors.right:          parent.right
                myGeoFenceController:   _geoFenceController
                flightMap:              editorMap
                visible:                _editingLayer == _layerGeoFence
            }
            // Rally Point Editor
            RallyPointEditorHeader {
                id:                     rallyPointHeader
                anchors.top:            rightControls.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
                anchors.left:           parent.left
                anchors.right:          parent.right
                visible:                _editingLayer == _layerRallyPoints
                controller:             _rallyPointController
            }
            RallyPointItemEditor {
                id:                     rallyPointEditor
                anchors.top:            rallyPointHeader.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
                anchors.left:           parent.left
                anchors.right:          parent.right
                visible:                _editingLayer == _layerRallyPoints && _rallyPointController.points.count
                rallyPoint:             _rallyPointController.currentRallyPoint
                controller:             _rallyPointController
            }
        }

        MapScale {
            id:                 mapScale
            anchors.margins:    ScreenTools.defaultFontPixelHeight * (0.66)
            anchors.bottom:     waypointValuesDisplay.visible ? waypointValuesDisplay.top : parent.bottom
            anchors.left:       parent.left
            mapControl:         editorMap
            visible:            _toolStripBottom < y
        }

        MissionItemStatus {
            id:                 waypointValuesDisplay
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.left:       parent.left
            height:             ScreenTools.defaultFontPixelHeight * 7
            maxWidth:           parent.width - rightPanel.width - x
            anchors.bottom:     parent.bottom
            missionItems:       _missionController.visualItems
            visible:            _editingLayer === _layerMission && (_toolStripBottom + mapScale.height) < y && QGroundControl.corePlugin.options.showMissionStatus
        }
    }

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
            message: qsTr("Are you sure you want to remove all items and create a new plan? ") +
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

    Component {
        id: clearVehicleMissionDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove all mission items and clear the mission from the vehicle?")
            function accept() {
                masterController.removeAllFromVehicle()
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

            Rectangle {
                width:              parent.width * 0.8
                height:             1
                color:              qgcPal.text
                opacity:            0.5
                Layout.fillWidth:   true
                Layout.columnSpan:  2
            }

            QGCButton {
                text:               qsTr("Load KML/SHP...")
                Layout.fillWidth:   true
                enabled:            !masterController.syncInProgress
                onClicked: {
                    masterController.loadShapeFromSelectedFile()
                    dropPanel.hide()
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
                                (_activeVehicle ?
                                     qsTr("You have unsaved changes. You should upload to your vehicle, or save to a file:") :
                                     qsTr("You have unsaved changes.")
                                ) :
                                qsTr("Plan File:")
            }

            GridLayout {
                id:                 sendSaveGrid
                columns:            2
                anchors.margins:    _margin
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:               qsTr("New...")
                    Layout.fillWidth:   true
                    enabled:            _visualItems.count > 1
                    onClicked:  {
                        dropPanel.hide()
                        _qgcView.showDialog(removeAllPromptDialog, qsTr("New Plan"), _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                    }
                }

                QGCButton {
                    text:               qsTr("Open...")
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
                    text:               qsTr("Save")
                    Layout.fillWidth:   true
                    enabled:            !masterController.syncInProgress && masterController.currentPlanFile !== ""
                    onClicked: {
                        dropPanel.hide()
                        if(masterController.currentPlanFile !== "") {
                            masterController.saveToCurrent()
                        } else {
                            masterController.saveToSelectedFile()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save As...")
                    Layout.fillWidth:   true
                    enabled:            !masterController.syncInProgress && _visualItems.count > 1
                    onClicked: {
                        dropPanel.hide()
                        masterController.saveToSelectedFile()
                    }
                }

                QGCButton {
                    text:               qsTr("Save Mission Waypoints As KML...")                    
                    Layout.columnSpan:  2
                    enabled:            !masterController.syncInProgress && _visualItems.count > 1
                    onClicked: {
                        // First point does not count
                        if (_visualItems.count < 2) {
                            _qgcView.showDialog(noItemForKML, qsTr("KML"), _qgcView.showDialogDefaultWidth, StandardButton.Cancel)
                            return
                        }
                        dropPanel.hide()
                        masterController.saveKmlToSelectedFile()
                    }
                }

                Rectangle {
                    width:              parent.width * 0.8
                    height:             1
                    color:              qgcPal.text
                    opacity:            0.5
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                }

                QGCButton {
                    text:               qsTr("Upload")
                    Layout.fillWidth:   true
                    enabled:            !masterController.offline && !masterController.syncInProgress && _visualItems.count > 1
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    onClicked: {
                        dropPanel.hide()
                        masterController.upload()
                    }
                }

                QGCButton {
                    text:               qsTr("Download")
                    Layout.fillWidth:   true
                    enabled:            !masterController.offline && !masterController.syncInProgress
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
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
                    text:               qsTr("Clear Vehicle Mission")
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                    enabled:            !masterController.offline && !masterController.syncInProgress
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    onClicked: {
                        dropPanel.hide()
                        _qgcView.showDialog(clearVehicleMissionDialog, text, _qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                    }
                }

            }
        }
    }
} // QGCVIew
