/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

Item {
    id: _root

    property bool planControlColapsed: false

    readonly property int   _decimalPlaces:             8
    readonly property real  _margin:                    ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real  _toolsMargin:               ScreenTools.defaultFontPixelWidth * 0.75
    readonly property real  _radius:                    ScreenTools.defaultFontPixelWidth  * 0.5
    readonly property real  _rightPanelWidth:           Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property var   _defaultVehicleCoordinate:  QtPositioning.coordinate(37.803784, -122.462276)
    readonly property bool  _waypointsOnlyMode:         QGroundControl.corePlugin.options.missionWaypointsOnly

    property bool   _airspaceEnabled:                    QGroundControl.airmapSupported ? (QGroundControl.settingsManager.airMapSettings.enableAirMap.rawValue && QGroundControl.airspaceManager.connected): false
    property var    _missionController:                 _planMasterController.missionController
    property var    _geoFenceController:                _planMasterController.geoFenceController
    property var    _rallyPointController:              _planMasterController.rallyPointController
    property var    _visualItems:                       _missionController.visualItems
    property bool   _lightWidgetBorders:                editorMap.isSatelliteMap
    property bool   _addWaypointOnClick:                false
    property bool   _addROIOnClick:                     false
    property bool   _singleComplexItem:                 _missionController.complexMissionItemNames.length === 1
    property int    _editingLayer:                      bar.currentIndex ? _layers[bar.currentIndex] : _layerMission
    property int    _toolStripBottom:                   toolStrip.height + toolStrip.y
    property var    _appSettings:                       QGroundControl.settingsManager.appSettings
    property var    _planViewSettings:                  QGroundControl.settingsManager.planViewSettings

    readonly property var       _layers:                [_layerMission, _layerGeoFence, _layerRallyPoints]

    readonly property int       _layerMission:              1
    readonly property int       _layerGeoFence:             2
    readonly property int       _layerRallyPoints:          3
    readonly property string    _armedVehicleUploadPrompt:  qsTr("Vehicle is currently armed. Do you want to upload the mission to the vehicle?")

    function mapCenter() {
        var coordinate = editorMap.center
        coordinate.latitude  = coordinate.latitude.toFixed(_decimalPlaces)
        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
        coordinate.altitude  = coordinate.altitude.toFixed(_decimalPlaces)
        return coordinate
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

    onVisibleChanged: {
        if(visible) {
            editorMap.zoomLevel = QGroundControl.flightMapZoom
            editorMap.center    = QGroundControl.flightMapPosition
            if (!_planMasterController.containsItems) {
                toolStrip.simulateClick(toolStrip.fileButtonIndex)
            }
        }
    }

    Connections {
        target: _appSettings ? _appSettings.defaultMissionItemAltitude : null
        onRawValueChanged: {
            if (_visualItems.count > 1) {
                mainWindow.showComponentDialog(applyNewAltitude, qsTr("Apply new alititude"), mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
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
                        activeVehicle.flightMode = activeVehicle.pauseFlightMode
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
        id: _planMasterController

        Component.onCompleted: {
            _planMasterController.start(false /* flyView */)
            _missionController.setCurrentPlanViewSeqNum(0, true)
            mainWindow.planMasterControllerPlan = _planMasterController
        }

        function waitingOnIncompleteDataMessage(save) {
            var saveOrUpload = save ? qsTr("Save") : qsTr("Upload")
            mainWindow.showMessageDialog(qsTr("Unable to %1").arg(saveOrUpload), qsTr("Plan has incomplete items. Complete all items and %1 again.").arg(saveOrUpload))
        }

        function waitingOnTerrainDataMessage(save) {
            var saveOrUpload = save ? qsTr("Save") : qsTr("Upload")
            mainWindow.showMessageDialog(qsTr("Unable to %1").arg(saveOrUpload), qsTr("Plan is waiting on terrain data from server for correct altitude values."))
        }

        function checkReadyForSaveUpload(save) {
            if (readyForSaveState() == VisualMissionItem.NotReadyForSaveData) {
                waitingOnIncompleteDataMessage(save)
                return false
            } else if (readyForSaveState() == VisualMissionItem.NotReadyForSaveTerrain) {
                waitingOnTerrainDataMessage(save)
                return false
            }
            return true
        }

        function upload() {
            if (!checkReadyForSaveUpload(false /* save */)) {
                return
            }
            if (activeVehicle && activeVehicle.armed && activeVehicle.flightMode === activeVehicle.missionFlightMode) {
                mainWindow.showComponentDialog(activeMissionUploadDialogComponent, qsTr("Plan Upload"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel)
            } else {
                sendToVehicle()
            }
        }

        function loadFromSelectedFile() {
            fileDialog.title =          qsTr("Select Plan File")
            fileDialog.planFiles =      true
            fileDialog.selectExisting = true
            fileDialog.nameFilters =    _planMasterController.loadNameFilters
            fileDialog.fileExtension =  _appSettings.planFileExtension
            fileDialog.fileExtension2 = _appSettings.missionFileExtension
            fileDialog.openForLoad()
        }

        function saveToSelectedFile() {
            if (!checkReadyForSaveUpload(true /* save */)) {
                return
            }
            fileDialog.title =          qsTr("Save Plan")
            fileDialog.planFiles =      true
            fileDialog.selectExisting = false
            fileDialog.nameFilters =    _planMasterController.saveNameFilters
            fileDialog.fileExtension =  _appSettings.planFileExtension
            fileDialog.fileExtension2 = _appSettings.missionFileExtension
            fileDialog.openForSave()
        }

        function fitViewportToItems() {
            mapFitFunctions.fitMapViewportToMissionItems()
        }

        function saveKmlToSelectedFile() {
            if (!checkReadyForSaveUpload(true /* save */)) {
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
            if (_visualItems && _visualItems.count !== 1) {
                mapFitFunctions.fitMapViewportToMissionItems()
            }
            _missionController.setCurrentPlanViewSeqNum(0, true)
        }
    }

    function insertSimpleItemAfterCurrent(coordinate) {
        var nextIndex = _missionController.currentPlanViewVIIndex + 1
        _missionController.insertSimpleMissionItem(coordinate, nextIndex, true /* makeCurrentItem */)
    }

    function insertROIAfterCurrent(coordinate) {
        var nextIndex = _missionController.currentPlanViewVIIndex + 1
        _missionController.insertROIMissionItem(coordinate, nextIndex, true /* makeCurrentItem */)
        _addROIOnClick = false
    }

    function insertCancelROIAfterCurrent() {
        var nextIndex = _missionController.currentPlanViewVIIndex + 1
        _missionController.insertCancelROIMissionItem(nextIndex, true /* makeCurrentItem */)
        _addROIOnClick = false
    }

    function insertComplexItemAfterCurrent(complexItemName) {
        var nextIndex = _missionController.currentPlanViewVIIndex + 1
        _missionController.insertComplexMissionItem(complexItemName, mapCenter(), nextIndex, true /* makeCurrentItem */)
    }

    function selectNextNotReady() {
        var foundCurrent = false
        for (var i=0; i<_missionController.visualItems.count; i++) {
            var vmi = _missionController.visualItems.get(i)
            if (vmi.readyForSaveState === VisualMissionItem.NotReadyForSaveData) {
                _missionController.setCurrentPlanViewSeqNum(vmi.sequenceNumber, true)
                break
            }
        }
    }

    property int _moveDialogMissionItemIndex

    QGCFileDialog {
        id:             fileDialog
        folder:         _appSettings ? _appSettings.missionSavePath : ""

        property bool planFiles: true    ///< true: working with plan files, false: working with kml file

        onAcceptedForSave: {
            if (planFiles) {
                _planMasterController.saveToFile(file)
            } else {
                _planMasterController.saveToKml(file)
            }
            close()
        }

        onAcceptedForLoad: {
            _planMasterController.loadFromFile(file)
            _planMasterController.fitViewportToItems()
            _missionController.setCurrentPlanViewSeqNum(0, true)
            close()
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

    Item {
        id:             panel
        anchors.fill:   parent

        FlightMap {
            id:                         editorMap
            anchors.fill:               parent
            mapName:                    "MissionEditor"
            allowGCSLocationCenter:     true
            allowVehicleLocationCenter: true
            planView:                   true

            zoomLevel:                  QGroundControl.flightMapZoom
            center:                     QGroundControl.flightMapPosition

            // This is the center rectangle of the map which is not obscured by tools
            property rect centerViewport:   Qt.rect(_leftToolWidth + _margin, _toolsMargin, editorMap.width - _leftToolWidth - _rightToolWidth - (_margin * 2), mapScale.y - _margin - _toolsMargin)

            property real _leftToolWidth:       toolStrip.x + toolStrip.width
            property real _rightToolWidth:      rightPanel.width + rightPanel.anchors.rightMargin

            // Initial map position duplicates Fly view position
            Component.onCompleted: editorMap.center = QGroundControl.flightMapPosition

            QGCMapPalette { id: mapPal; lightColors: editorMap.isSatelliteMap }

            onZoomLevelChanged: {
                QGroundControl.flightMapZoom = zoomLevel
                updateAirspace(false)
            }
            onCenterChanged: {
                QGroundControl.flightMapPosition = center
                updateAirspace(false)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // Take focus to close any previous editing
                    editorMap.focus = true
                    var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y), false /* clipToViewPort */)
                    coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                    coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                    coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)

                    switch (_editingLayer) {
                    case _layerMission:
                        if (_addWaypointOnClick) {
                            insertSimpleItemAfterCurrent(coordinate)
                        } else if (_addROIOnClick) {
                            _addROIOnClick = false
                            insertROIAfterCurrent(coordinate)
                        }

                        break
                    case _layerRallyPoints:
                        if (_rallyPointController.supported && _addWaypointOnClick) {
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
                    onClicked:  _missionController.setCurrentPlanViewSeqNum(sequenceNumber, false)
                    visible:    _editingLayer == _layerMission
                }
            }

            // Add lines between waypoints
            MissionLineView {
                showSpecialVisual:  _missionController.isROIBeginCurrentItem
                model:              _editingLayer == _layerMission ? _missionController.waypointLines : undefined
            }

            // Direction arrows in waypoint lines
            MapItemView {
                model: _editingLayer == _layerMission ? _missionController.directionArrows : undefined

                delegate: MapLineArrow {
                    fromCoord:      object ? object.coordinate1 : undefined
                    toCoord:        object ? object.coordinate2 : undefined
                    arrowPosition:  3
                    z:              QGroundControl.zOrderWaypointLines + 1
                }
            }

            // Incomplete segment lines
            MapItemView {
                model: _editingLayer == _layerMission ? _missionController.incompleteComplexItemLines : undefined

                delegate: MapPolyline {
                    path:       [ object.coordinate1, object.coordinate2 ]
                    line.width: 1
                    line.color: "red"
                    z:          QGroundControl.zOrderWaypointLines
                }
            }

            // UI for splitting the current segment
            MapQuickItem {
                id:             splitSegmentItem
                anchorPoint.x:  sourceItem.width / 2
                anchorPoint.y:  sourceItem.height / 2
                z:              QGroundControl.zOrderWaypointLines + 1
                visible:        _editingLayer == _layerMission

                sourceItem: SplitIndicator {
                    onClicked:  _missionController.insertSimpleMissionItem(splitSegmentItem.coordinate,
                                                                           _missionController.currentPlanViewVIIndex,
                                                                           true /* makeCurrentItem */)
                }

                function _updateSplitCoord() {
                    if (_missionController.splitSegment) {
                        var distance = _missionController.splitSegment.coordinate1.distanceTo(_missionController.splitSegment.coordinate2)
                        var azimuth = _missionController.splitSegment.coordinate1.azimuthTo(_missionController.splitSegment.coordinate2)
                        splitSegmentItem.coordinate = _missionController.splitSegment.coordinate1.atDistanceAndAzimuth(distance / 2, azimuth)
                    } else {
                        coordinate = QtPositioning.coordinate()
                    }
                }

                Connections {
                    target:                 _missionController
                    onSplitSegmentChanged:  splitSegmentItem._updateSplitCoord()
                }

                Connections {
                    target:                 _missionController.splitSegment
                    onCoordinate1Changed:   splitSegmentItem._updateSplitCoord()
                    onCoordinate2Changed:   splitSegmentItem._updateSplitCoord()
                }
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
        }

        //-----------------------------------------------------------
        // Left tool strip
        ToolStrip {
            id:                 toolStrip
            anchors.margins:    _toolsMargin
            anchors.left:       parent.left
            anchors.top:        parent.top
            z:                  QGroundControl.zOrderWidgets
            maxHeight:          parent.height - toolStrip.y

            //readonly property int flyButtonIndex:       0
            readonly property int fileButtonIndex:      0
            readonly property int takeoffButtonIndex:   1
            readonly property int waypointButtonIndex:  2
            readonly property int roiButtonIndex:       3
            readonly property int patternButtonIndex:   4
            readonly property int landButtonIndex:      5
            readonly property int centerButtonIndex:    6

            property bool _isRallyLayer:    _editingLayer == _layerRallyPoints
            property bool _isMissionLayer:  _editingLayer == _layerMission

            model: [
                /*{
                    name:               qsTr("Fly"),
                    iconSource:         "/qmlimages/PaperPlane.svg",
                    buttonEnabled:      true,
                    buttonVisible:      true,
                },*/
                {
                    name:               qsTr("File"),
                    iconSource:         "/qmlimages/MapSync.svg",
                    buttonEnabled:      !_planMasterController.syncInProgress,
                    buttonVisible:      true,
                    showAlternateIcon:  _planMasterController.dirty,
                    alternateIconSource:"/qmlimages/MapSyncChanged.svg",
                    dropPanelComponent: syncDropPanel
                },
                {
                    name:               qsTr("Takeoff"),
                    iconSource:         "/res/takeoff.svg",
                    buttonEnabled:      _missionController.isInsertTakeoffValid,
                    buttonVisible:      _isMissionLayer
                },
                {
                    name:               _editingLayer == _layerRallyPoints ? qsTr("Rally Point") : qsTr("Waypoint"),
                    iconSource:         "/qmlimages/MapAddMission.svg",
                    buttonEnabled:      _isRallyLayer ? true : _missionController.flyThroughCommandsAllowed,
                    buttonVisible:      _isRallyLayer || _isMissionLayer,
                    toggle:             true,
                    checked:            _addWaypointOnClick
                },
                {
                    name:               _missionController.isROIActive ? qsTr("Cancel ROI") : qsTr("ROI"),
                    iconSource:         "/qmlimages/MapAddMission.svg",
                    buttonEnabled:      true,
                    buttonVisible:      _isMissionLayer,
                    toggle:             !_missionController.isROIActive
                },
                {
                    name:               _singleComplexItem ? _missionController.complexMissionItemNames[0] : qsTr("Pattern"),
                    iconSource:         "/qmlimages/MapDrawShape.svg",
                    buttonEnabled:      _missionController.flyThroughCommandsAllowed,
                    buttonVisible:      _isMissionLayer,
                    dropPanelComponent: _singleComplexItem ? undefined : patternDropPanel
                },
                {
                    name:               _planMasterController.controllerVehicle.fixedWing ? qsTr("Land") : qsTr("Return"),
                    iconSource:         "/res/rtl.svg",
                    buttonEnabled:      _missionController.isInsertLandValid,
                    buttonVisible:      _isMissionLayer
                },
                {
                    name:               qsTr("Center"),
                    iconSource:         "/qmlimages/MapCenter.svg",
                    buttonEnabled:      true,
                    buttonVisible:      true,
                    dropPanelComponent: centerMapDropPanel
                }
            ]

            function allAddClickBoolsOff() {
                _addROIOnClick =        false
                _addWaypointOnClick =   false
            }

            onClicked: {
                switch (index) {
                /*case flyButtonIndex:
                    mainWindow.showFlyView()
                    break*/
                case takeoffButtonIndex:
                    allAddClickBoolsOff()
                    _missionController.insertTakeoffItem(mapCenter(), _missionController.currentMissionIndex, true /* makeCurrentItem */)
                    break
                case waypointButtonIndex:
                    if (_addWaypointOnClick) {
                        allAddClickBoolsOff()
                        setChecked(index, false)
                    } else {
                        allAddClickBoolsOff()
                        _addWaypointOnClick = checked
                    }
                    break
                case roiButtonIndex:
                    if (_addROIOnClick) {
                        allAddClickBoolsOff()
                        setChecked(index, false)
                    } else {
                        allAddClickBoolsOff()
                        if (_missionController.isROIActive) {
                            insertCancelROIAfterCurrent()
                        } else {
                            _addROIOnClick = checked
                        }
                    }
                    break
                case patternButtonIndex:
                    allAddClickBoolsOff()
                    if (_singleComplexItem) {
                        insertComplexItemAfterCurrent(_missionController.complexMissionItemNames[0])
                    }
                    break
                case landButtonIndex:
                    allAddClickBoolsOff()
                    _missionController.insertLandItem(mapCenter(), _missionController.currentMissionIndex, true /* makeCurrentItem */)
                    break
                }
            }

            onDropped: {
                allAddClickBoolsOff()
            }
        }

        //-----------------------------------------------------------
        // Right pane for mission editing controls
        Rectangle {
            id:                 rightPanel
            height:             parent.height
            width:              _rightPanelWidth
            color:              qgcPal.window
            opacity:            planExpanded.visible ? 0.2 : 0
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            anchors.rightMargin: _toolsMargin
        }
        //-------------------------------------------------------
        // Right Panel Controls
        Item {
            anchors.fill:           rightPanel
            anchors.topMargin:      _toolsMargin
            DeadMouseArea {
                anchors.fill:   parent
            }
            Column {
                id:                 rightControls
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
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
                    height:     (!planControlColapsed || !_airspaceEnabled) ? bar.height + ScreenTools.defaultFontPixelHeight : 0
                    color:      qgcPal.missionItemEditor
                    radius:     _radius
                    visible:    (!planControlColapsed || !_airspaceEnabled) && QGroundControl.corePlugin.options.enablePlanViewSelector
                    Item {
                        height:             bar.height
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        anchors.margins:    ScreenTools.defaultFontPixelWidth
                        anchors.verticalCenter: parent.verticalCenter
                        QGCTabBar {
                            id:             bar
                            width:          parent.width
                            anchors.centerIn: parent
                            Component.onCompleted: {
                                currentIndex = 0
                            }
                            QGCTabButton {
                                text:       qsTr("Mission")
                            }
                            QGCTabButton {
                                text:       qsTr("Fence")
                                enabled:    _geoFenceController.supported
                            }
                            QGCTabButton {
                                text:       qsTr("Rally")
                                enabled:    _rallyPointController.supported
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
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
                anchors.bottom:         parent.bottom
                anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * 0.25
                visible:                _editingLayer == _layerMission && !planControlColapsed
                QGCListView {
                    id:                 missionItemEditorListView
                    anchors.fill:       parent
                    spacing:            ScreenTools.defaultFontPixelHeight / 4
                    orientation:        ListView.Vertical
                    model:              _missionController.visualItems
                    cacheBuffer:        Math.max(height * 2, 0)
                    clip:               true
                    currentIndex:       _missionController.currentPlanViewSeqNum
                    highlightMoveDuration: 250
                    visible:            _editingLayer == _layerMission && !planControlColapsed
                    //-- List Elements
                    delegate: MissionItemEditor {
                        map:            editorMap
                        masterController:  _planMasterController
                        missionItem:    object
                        width:          parent.width
                        readOnly:       false
                        onClicked:      _missionController.setCurrentPlanViewSeqNum(object.sequenceNumber, false)
                        onRemove: {
                            var removeIndex = index
                            _missionController.removeMissionItem(removeIndex)
                            if (removeIndex >= _missionController.visualItems.count) {
                                removeIndex--
                            }
                            _missionController.setCurrentPlanViewSeqNum(removeIndex, true)
                        }
                        onSelectNextNotReadyItem:   selectNextNotReady()
                    }
                }
            }
            // GeoFence Editor
            GeoFenceEditor {
                anchors.top:            rightControls.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
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
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
                anchors.left:           parent.left
                anchors.right:          parent.right
                visible:                _editingLayer == _layerRallyPoints
                controller:             _rallyPointController
            }
            RallyPointItemEditor {
                id:                     rallyPointEditor
                anchors.top:            rallyPointHeader.bottom
                anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.25
                anchors.left:           parent.left
                anchors.right:          parent.right
                visible:                _editingLayer == _layerRallyPoints && _rallyPointController.points.count
                rallyPoint:             _rallyPointController.currentRallyPoint
                controller:             _rallyPointController
            }
        }

        MissionItemStatus {
            id:                 waypointValuesDisplay
            anchors.margins:    _toolsMargin
            anchors.left:       toolStrip.right
            anchors.bottom:     mapScale.top
            height:             ScreenTools.defaultFontPixelHeight * 7
            maxWidth:           rightPanel.x - x - anchors.margins
            missionItems:       _missionController.visualItems
            visible:            _internalVisible && _editingLayer === _layerMission && QGroundControl.corePlugin.options.showMissionStatus

            onSetCurrentSeqNum: _missionController.setCurrentPlanViewSeqNum(seqNum, true)

            property bool _internalVisible: _planViewSettings.showMissionItemStatus.rawValue

            function toggleVisible() {
                _internalVisible = !_internalVisible
                _planViewSettings.showMissionItemStatus.rawValue = _internalVisible
            }
        }


        MapScale {
            id:                     mapScale
            anchors.margins:        _toolsMargin
            anchors.bottom:         parent.bottom
            anchors.left:           toolStrip.right
            mapControl:             editorMap
            buttonsOnLeft:          true
            terrainButtonVisible:   _editingLayer === _layerMission
            terrainButtonChecked:   waypointValuesDisplay.visible
            onTerrainButtonClicked: waypointValuesDisplay.toggleVisible()
        }
    }

    Component {
        id: syncLoadFromVehicleOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading from the Vehicle will lose these changes. Are you sure you want to load from the Vehicle?")
            function accept() {
                hideDialog()
                _planMasterController.loadFromVehicle()
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
                _planMasterController.loadFromSelectedFile()
            }
        }
    }

    property var createPlanRemoveAllPromptDialogMapCenter
    property var createPlanRemoveAllPromptDialogPlanCreator
    Component {
        id: createPlanRemoveAllPromptDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove current plan and create a new plan? ")
            function accept() {
                createPlanRemoveAllPromptDialogPlanCreator.createPlan(createPlanRemoveAllPromptDialogMapCenter)
                hideDialog()
            }
        }
    }

    Component {
        id: clearVehicleMissionDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove all mission items and clear the mission from the vehicle?")
            function accept() {
                _planMasterController.removeAllFromVehicle()
                _missionController.setCurrentPlanViewSeqNum(0, true)
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
                        insertComplexItemAfterCurrent(modelData)
                        dropPanel.hide()
                    }
                }
            }
        } // Column
    }

    Component {
        id: syncDropPanel

        ColumnLayout {
            id:         columnHolder
            spacing:    _margin

            property string _overwriteText: (_editingLayer == _layerMission) ? qsTr("Mission overwrite") : ((_editingLayer == _layerGeoFence) ? qsTr("GeoFence overwrite") : qsTr("Rally Points overwrite"))

            QGCLabel {
                id:                 unsavedChangedLabel
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               activeVehicle ?
                                        qsTr("You have unsaved changes. You should upload to your vehicle, or save to a file.") :
                                        qsTr("You have unsaved changes.")
                visible:            _planMasterController.dirty
            }

            SectionHeader {
                id:                 createSection
                Layout.fillWidth:   true
                text:               qsTr("Create Plan")
                showSpacer:         false
            }

            GridLayout {
                columns:            2
                columnSpacing:      _margin
                rowSpacing:         _margin
                Layout.fillWidth:   true
                visible:            createSection.visible

                Repeater {
                    model: _planMasterController.planCreators

                    Rectangle {
                        id:     button
                        width:  ScreenTools.defaultFontPixelHeight * 7
                        height: planCreatorNameLabel.y + planCreatorNameLabel.height
                        color:  button.pressed || button.highlighted ? qgcPal.buttonHighlight : qgcPal.button

                        property bool highlighted: mouseArea.containsMouse
                        property bool pressed:     mouseArea.pressed

                        Image {
                            id:                 planCreatorImage
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            source:             object.imageResource
                            sourceSize.width:   width
                            fillMode:           Image.PreserveAspectFit
                            mipmap:             true
                        }

                        QGCLabel {
                            id:                     planCreatorNameLabel
                            anchors.top:            planCreatorImage.bottom
                            anchors.left:           parent.left
                            anchors.right:          parent.right
                            horizontalAlignment:    Text.AlignHCenter
                            text:                   object.name
                            color:                  button.pressed || button.highlighted ? qgcPal.buttonHighlightText : qgcPal.buttonText
                        }

                        QGCMouseArea {
                            id:                 mouseArea
                            anchors.fill:       parent
                            hoverEnabled:       true
                            preventStealing:    true
                            onClicked:          {
                                if (_planMasterController.containsItems) {
                                    createPlanRemoveAllPromptDialogMapCenter = _mapCenter()
                                    createPlanRemoveAllPromptDialogPlanCreator = object
                                    mainWindow.showComponentDialog(createPlanRemoveAllPromptDialog, qsTr("Create Plan"), mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                                } else {
                                    object.createPlan(_mapCenter())
                                }
                                dropPanel.hide()
                            }

                            function _mapCenter() {
                                var centerPoint = Qt.point(editorMap.centerViewport.left + (editorMap.centerViewport.width / 2), editorMap.centerViewport.top + (editorMap.centerViewport.height / 2))
                                return editorMap.toCoordinate(centerPoint, false /* clipToViewPort */)
                            }
                        }
                    }
                }
            }

            SectionHeader {
                id:                 storageSection
                Layout.fillWidth:   true
                text:               qsTr("Storage")
            }

            GridLayout {
                columns:            3
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth
                visible:            storageSection.visible

                /*QGCButton {
                    text:               qsTr("New...")
                    Layout.fillWidth:   true
                    onClicked:  {
                        dropPanel.hide()
                        if (_planMasterController.containsItems) {
                            mainWindow.showComponentDialog(removeAllPromptDialog, qsTr("New Plan"), mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                        }
                    }
                }*/

                QGCButton {
                    text:               qsTr("Open...")
                    Layout.fillWidth:   true
                    enabled:            !_planMasterController.syncInProgress
                    onClicked: {
                        dropPanel.hide()
                        if (_planMasterController.dirty) {
                            mainWindow.showComponentDialog(syncLoadFromFileOverwrite, columnHolder._overwriteText, mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _planMasterController.loadFromSelectedFile()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save")
                    Layout.fillWidth:   true
                    enabled:            !_planMasterController.syncInProgress && _planMasterController.currentPlanFile !== ""
                    onClicked: {
                        dropPanel.hide()
                        if(_planMasterController.currentPlanFile !== "") {
                            _planMasterController.saveToCurrent()
                        } else {
                            _planMasterController.saveToSelectedFile()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save As...")
                    Layout.fillWidth:   true
                    enabled:            !_planMasterController.syncInProgress && _planMasterController.containsItems
                    onClicked: {
                        dropPanel.hide()
                        _planMasterController.saveToSelectedFile()
                    }
                }

                QGCButton {
                    Layout.columnSpan:  3
                    Layout.fillWidth:   true
                    text:               qsTr("Save Mission Waypoints As KML...")
                    enabled:            !_planMasterController.syncInProgress && _visualItems.count > 1
                    onClicked: {
                        // First point does not count
                        if (_visualItems.count < 2) {
                            mainWindow.showComponentDialog(noItemForKML, qsTr("KML"), mainWindow.showDialogDefaultWidth, StandardButton.Cancel)
                            return
                        }
                        dropPanel.hide()
                        _planMasterController.saveKmlToSelectedFile()
                    }
                }
            }

            SectionHeader {
                id:                 vehicleSection
                Layout.fillWidth:   true
                text:               qsTr("Vehicle")
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            _margin
                visible:            vehicleSection.visible

                QGCButton {
                    text:               qsTr("Upload")
                    Layout.fillWidth:   true
                    enabled:            !_planMasterController.offline && !_planMasterController.syncInProgress && _planMasterController.containsItems
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    onClicked: {
                        dropPanel.hide()
                        _planMasterController.upload()
                    }
                }

                QGCButton {
                    text:               qsTr("Download")
                    Layout.fillWidth:   true
                    enabled:            !_planMasterController.offline && !_planMasterController.syncInProgress
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    onClicked: {
                        dropPanel.hide()
                        if (_planMasterController.dirty) {
                            mainWindow.showComponentDialog(syncLoadFromVehicleOverwrite, columnHolder._overwriteText, mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _planMasterController.loadFromVehicle()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Clear")
                    Layout.fillWidth:   true
                    Layout.columnSpan:  2
                    enabled:            !_planMasterController.offline && !_planMasterController.syncInProgress
                    visible:            !QGroundControl.corePlugin.options.disableVehicleConnection
                    onClicked: {
                        dropPanel.hide()
                        mainWindow.showComponentDialog(clearVehicleMissionDialog, text, mainWindow.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                    }
                }
            }
        }
    }
}
