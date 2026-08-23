/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// Fly-view 2D/3D map: the GeoMap-engine counterpart of FlyViewMap.
/// Fly-view specific map content (vehicle items, trajectory, ...) lands here.
GeoMap {
    id: root

    allowGCSLocationCenter: true
    allowVehicleLocationCenter: true
    keepVehicleCentered: QGroundControl.settingsManager.flyViewSettings.keepMapCenteredOnVehicle.rawValue

    // Guided-action indicators for the map click drop panel (FlyViewMapClickDropPanel)
    readonly property var gotoIndicator: gotoLocationItem
    readonly property var orbitIndicator: orbitMapCircle

    readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property var _missionController: globals.planMasterControllerFlyView ? globals.planMasterControllerFlyView.missionController : null
    readonly property var _rallyPointController: globals.planMasterControllerFlyView ? globals.planMasterControllerFlyView.rallyPointController : null
    readonly property var _geoFenceController: globals.planMasterControllerFlyView ? globals.planMasterControllerFlyView.geoFenceController : null
    readonly property var _flyViewSettings: QGroundControl.settingsManager.flyViewSettings

    // DEM height minus vehicle-reported AMSL, sampled at home for the active
    // vehicle. Computed once here and shared by GeoMapMissionPath and every
    // GeoMapMissionItems marker instead of each re-sampling the same DEM
    // lookup independently (see GeoMapVehicleItem for the same correction
    // applied per-vehicle in the marker Repeater below, where it can't be
    // shared since each vehicle has its own home position).
    property real _activeVehicleHomeTerrainBias: 0

    function _updateActiveVehicleHomeTerrainBias() {
        if (root.surfaceModel && root._activeVehicle && root._activeVehicle.homePosition.isValid && !isNaN(root._activeVehicle.homePosition.altitude)) {
            root._activeVehicleHomeTerrainBias = root.surfaceModel.terrainHeightAt(root._activeVehicle.homePosition) - root._activeVehicle.homePosition.altitude
        } else {
            root._activeVehicleHomeTerrainBias = 0
        }
    }

    on_ActiveVehicleChanged: {
        _updateActiveVehicleHomeTerrainBias()
        _fuzzyVehicleDisplayAltitude = NaN
        _updateFuzzyVehicleDisplayAltitude()
    }
    onSurfaceModelChanged: _updateActiveVehicleHomeTerrainBias()

    // Deadbanded copy of the vehicle's bias-corrected display altitude so
    // live-bound orbit visuals don't rebuild their ring geometry on every
    // sub-meter telemetry altitude tick
    property real _fuzzyVehicleDisplayAltitude: NaN

    readonly property real _fuzzyAltitudeDeadbandMeters: 1

    function _updateFuzzyVehicleDisplayAltitude() {
        if (!root._activeVehicle || isNaN(root._activeVehicle.coordinate.altitude)) {
            root._fuzzyVehicleDisplayAltitude = NaN
            return
        }
        const displayAltitude = root._activeVehicle.coordinate.altitude + root._activeVehicleHomeTerrainBias
        if (isNaN(root._fuzzyVehicleDisplayAltitude) || Math.abs(displayAltitude - root._fuzzyVehicleDisplayAltitude) > root._fuzzyAltitudeDeadbandMeters) {
            root._fuzzyVehicleDisplayAltitude = displayAltitude
        }
    }

    on_ActiveVehicleHomeTerrainBiasChanged: _updateFuzzyVehicleDisplayAltitude()

    function _withFuzzyVehicleAltitude(coord) {
        if (!isNaN(root._fuzzyVehicleDisplayAltitude)) {
            return QtPositioning.coordinate(coord.latitude, coord.longitude, root._fuzzyVehicleDisplayAltitude)
        }
        return QtPositioning.coordinate(coord.latitude, coord.longitude)
    }

    // Guided commands hold the vehicle's current altitude; same bias-corrected
    // AMSL as GeoMapVehicleItem so indicators line up with the rendered vehicle
    function _withVehicleAltitude(coord) {
        if (root._activeVehicle && !isNaN(root._activeVehicle.coordinate.altitude)) {
            return QtPositioning.coordinate(coord.latitude, coord.longitude,
                                            root._activeVehicle.coordinate.altitude + root._activeVehicleHomeTerrainBias)
        }
        return QtPositioning.coordinate(coord.latitude, coord.longitude)
    }

    Connections {
        target: root.surfaceModel

        function onTerrainHeightsChanged() {
            root._updateActiveVehicleHomeTerrainBias()
        }
    }

    Connections {
        target: root._activeVehicle

        function onHomePositionChanged() {
            root._updateActiveVehicleHomeTerrainBias()
        }

        function onCoordinateChanged() {
            root._updateFuzzyVehicleDisplayAltitude()
        }
    }

    // Vehicle home (launch) location
    GeoMapPin {
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._activeVehicle ? root._activeVehicle.homePosition : QtPositioning.coordinate()
        visible: root._activeVehicle && root._activeVehicle.homePosition.isValid
        label: qsTr("L")
    }

    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        GeoMapVehicleItem {
            scene: root.scene
            surfaceModel: root.surfaceModel
            vehicle: object
        }
    }

    GeoMapFlightPath {
        scene: root.scene
        surfaceModel: root.surfaceModel
        vehicle: root._activeVehicle
    }

    GeoMapMissionPath {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        vehicle: root._activeVehicle
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }

    GeoMapMissionItems {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }

    GeoMapMissionDirectionArrows {
        scene: root.scene
        surfaceModel: root.surfaceModel
        missionController: root._missionController
        homeTerrainBias: root._activeVehicleHomeTerrainBias
    }

    GeoMapGeoFenceVisuals {
        scene: root.scene
        surfaceModel: root.surfaceModel
        geoFenceController: root._geoFenceController
        homePosition: root._activeVehicle && root._activeVehicle.homePosition.isValid ? root._activeVehicle.homePosition : QtPositioning.coordinate()
    }

    // Rally points
    Repeater {
        model: root._rallyPointController ? root._rallyPointController.points : undefined

        GeoMapMissionLabel {
            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinate: object.coordinate
            label: qsTr("R", "rally point map item label")
        }
    }

    // Proximity sensor arcs per vehicle
    Repeater {
        model: QGroundControl.multiVehicleManager.vehicles

        FlyViewGeoProximityRadar {
            scene: root.scene
            surfaceModel: root.surfaceModel
            vehicle: object
        }
    }

    // ADS-B traffic
    Repeater {
        model: QGroundControl.adsbVehicleManager.adsbVehicles

        GeoMapADSBVehicleItem {
            scene: root.scene
            surfaceModel: root.surfaceModel
            coordinate: object.coordinate
            altitude: object.altitude
            callsign: object.callsign
            heading: object.heading
            alert: object.alert
            size: ScreenTools.defaultFontPixelHeight * 2.5
            homeTerrainBias: root._activeVehicleHomeTerrainBias
        }
    }

    // Camera trigger points: batched sprite layer (single draw call), since
    // surveys can produce thousands of points
    GeoMapSpriteLayer {
        scene: root.scene
        surfaceModel: root.surfaceModel
        model: root._activeVehicle ? root._activeVehicle.cameraTriggerPoints : null
        sprite: triggerIconSource
        spriteSize: triggerIcon.width

        CameraTriggerIcon { id: triggerIcon }
        ShaderEffectSource {
            id: triggerIconSource
            sourceItem: triggerIcon
            hideSource: true
            // Render at physical resolution: the default (logical-size) texture
            // is upscaled on hidpi displays and looks blurry
            textureSize: Qt.size(triggerIcon.width * Screen.devicePixelRatio, triggerIcon.height * Screen.devicePixelRatio)
        }
    }

    // GoTo Location forward flight circle visuals (FlyViewMap parity, outline
    // only: no drag-to-edit radius on the GeoMap engine yet)
    GeoMapCircle {
        id: fwdFlightGotoMapCircle
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: _fwdFlightGotoCircleModel.center
        radiusMeters: _fwdFlightGotoCircleModel.radius.rawValue
        // Loiter happens at the goto altitude (carried in the center coordinate)
        altitudeMode: (center && !isNaN(center.altitude)) ? GeoMapItem.Absolute : GeoMapItem.ClampToGround
        // PX4 ignores the commanded loiter radius (flies NAV_LOITER_RAD), so the circle size is unknown
        visible: gotoLocationItem.visible && root._activeVehicle &&
                 !root._activeVehicle.px4Firmware &&
                 root._activeVehicle.inFwdFlight &&
                 !root._activeVehicle.orbitActive

        property alias coordinate: _fwdFlightGotoCircleModel.center
        property alias radius: _fwdFlightGotoCircleModel.radius
        property alias clockwiseRotation: _fwdFlightGotoCircleModel.clockwiseRotation

        Component.onCompleted: globals.guidedControllerFlyView.fwdFlightGotoMapCircle = this

        Binding {
            target: _fwdFlightGotoCircleModel
            property: "center"
            value: gotoLocationItem.coordinate
        }

        function startLoiterRadiusEdit() {
            _fwdFlightGotoCircleModel.interactive = true
        }

        // Called when loiter edit is confirmed
        function actionConfirmed() {
            _fwdFlightGotoCircleModel.interactive = false
            _fwdFlightGotoCircleModel._commitRadius()
        }

        // Called when loiter edit is cancelled
        function actionCancelled() {
            _fwdFlightGotoCircleModel.interactive = false
            _fwdFlightGotoCircleModel._restoreRadius()
        }

        QGCMapCircle {
            id: _fwdFlightGotoCircleModel
            interactive: false
            showRotation: true
            clockwiseRotation: true

            property real _defaultLoiterRadius: root._flyViewSettings.forwardFlightGoToLocationLoiterRad.rawValue
            property real _committedRadius

            onCenterChanged: {
                // Don't commit the radius in case this operation is undone
                radius.rawValue = _defaultLoiterRadius
            }

            Component.onCompleted: {
                radius.rawValue = _defaultLoiterRadius
                _commitRadius()
            }

            function _commitRadius() {
                _committedRadius = radius.rawValue
            }

            function _restoreRadius() {
                radius.rawValue = _committedRadius
            }
        }
    }

    // GoTo Location visuals
    GeoMapMissionLabel {
        id: gotoLocationItem
        scene: root.scene
        surfaceModel: root.surfaceModel
        visible: false
        checked: true
        label: qsTr("Go here", "Go to location waypoint")
        // Goto holds the vehicle's current altitude, so render there in 3D
        altitudeMode: isNaN(coordinate.altitude) ? GeoMapItem.ClampToGround : GeoMapItem.Absolute

        readonly property bool inGotoFlightMode: root._activeVehicle ? root._activeVehicle.flightMode === root._activeVehicle.gotoFlightMode : false

        property var _committedCoordinate: null

        onInGotoFlightModeChanged: {
            if (!inGotoFlightMode && gotoLocationItem.visible) {
                // Hide goto indicator when vehicle falls out of guided mode
                hide()
            }
        }

        function show(coord) {
            gotoLocationItem.coordinate = root._withVehicleAltitude(coord)
            gotoLocationItem.visible = true
        }

        function hide() {
            gotoLocationItem.visible = false
        }

        function actionConfirmed() {
            _commitCoordinate()

            // Commit the new radius which possibly changed
            fwdFlightGotoMapCircle.actionConfirmed()

            // We leave the indicator visible. The handling for onInGuidedModeChanged will hide it.
        }

        function actionCancelled() {
            _restoreCoordinate()

            // Also restore the loiter radius
            fwdFlightGotoMapCircle.actionCancelled()
        }

        function _commitCoordinate() {
            // Must deep copy
            _committedCoordinate = QtPositioning.coordinate(coordinate.latitude, coordinate.longitude, coordinate.altitude)
        }

        function _restoreCoordinate() {
            if (_committedCoordinate) {
                coordinate = _committedCoordinate
            } else {
                hide()
            }
        }
    }

    // Orbit editing visuals (outline only: no drag-to-edit radius on the
    // GeoMap engine yet, the confirm slider still sets the altitude)
    GeoMapCircle {
        id: orbitMapCircle
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: _orbitCircleModel.center
        radiusMeters: _orbitCircleModel.radius.rawValue
        altitudeMode: (center && !isNaN(center.altitude)) ? GeoMapItem.Absolute : GeoMapItem.ClampToGround
        visible: false

        property alias clockwiseRotation: _orbitCircleModel.clockwiseRotation
        readonly property real defaultRadius: 30

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) {
                if (!activeVehicle) {
                    orbitMapCircle.visible = false
                }
            }
        }

        function show(coord) {
            _orbitCircleModel.radius.rawValue = defaultRadius
            _orbitCircleModel.center = root._withVehicleAltitude(coord)
            orbitMapCircle.visible = true
        }

        function hide() {
            orbitMapCircle.visible = false
        }

        function actionConfirmed() {
            // Live orbit status is handled by telemetry so we hide here and telemetry will show again.
            hide()
        }

        function actionCancelled() {
            hide()
        }

        function radius() {
            return _orbitCircleModel.radius.rawValue
        }

        Component.onCompleted: globals.guidedControllerFlyView.orbitMapCircle = orbitMapCircle

        QGCMapCircle {
            id: _orbitCircleModel
            interactive: false
            radius.rawValue: 30
            showRotation: true
            clockwiseRotation: true
        }
    }

    // ROI Location visuals
    GeoMapMissionLabel {
        id: roiLocationItem
        scene: root.scene
        surfaceModel: root.surfaceModel
        visible: root._activeVehicle && root._activeVehicle.isROIEnabled
        checked: true
        label: qsTr("ROI here", "Make this a Region Of Interest")

        Connections {
            target: root._activeVehicle
            function onRoiCoordChanged(centerCoord) {
                roiLocationItem.coordinate = centerCoord
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: (position) => {
                const windowPosition = mapToItem(globals.parent, position.x, position.y)
                const dropPanel = roiEditDropPanelComponent.createObject(mainWindow, { clickRect: Qt.rect(windowPosition.x, windowPosition.y, 0, 0) })
                dropPanel.open()
            }
        }
    }

    // Orbit telemetry visuals. ORBIT_EXECUTION_STATUS carries no altitude, so
    // render at the vehicle's altitude (the vehicle flies on this ring)
    GeoMapCircle {
        id: orbitTelemetryCircle
        scene: root.scene
        surfaceModel: root.surfaceModel
        center: root._activeVehicle ? root._withFuzzyVehicleAltitude(root._activeVehicle.orbitMapCircle.center) : QtPositioning.coordinate()
        radiusMeters: root._activeVehicle ? root._activeVehicle.orbitMapCircle.radius.rawValue : 0
        altitudeMode: (center && !isNaN(center.altitude)) ? GeoMapItem.Absolute : GeoMapItem.ClampToGround
        visible: root._activeVehicle ? root._activeVehicle.orbitActive : false
    }

    GeoMapMissionLabel {
        id: orbitCenterIndicator
        scene: root.scene
        surfaceModel: root.surfaceModel
        coordinate: root._activeVehicle ? root._withFuzzyVehicleAltitude(root._activeVehicle.orbitMapCircle.center) : QtPositioning.coordinate()
        altitudeMode: isNaN(coordinate.altitude) ? GeoMapItem.ClampToGround : GeoMapItem.Absolute
        visible: orbitTelemetryCircle.visible && !gotoLocationItem.visible
        checked: true
        label: qsTr("Orbit", "Orbit waypoint")
    }

    QGCPopupDialogFactory {
        id: roiEditPositionDialogFactory

        dialogComponent: roiEditPositionDialogComponent
    }

    Component {
        id: roiEditPositionDialogComponent

        EditPositionDialog {
            title: qsTr("Edit ROI Position")
            coordinate: roiLocationItem.coordinate

            readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            // The ROI belongs to the vehicle the dialog was opened for; close
            // if that vehicle goes away or the active vehicle changes
            on_ActiveVehicleChanged: close()

            onCoordinateChanged: {
                roiLocationItem.coordinate = coordinate
                _activeVehicle.guidedModeROI(coordinate, _activeVehicle.roiRelativeAltitudeMeters)
            }
        }
    }

    Component {
        id: roiEditDropPanelComponent

        DropPanel {
            id: roiEditDropPanel

            readonly property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

            // The ROI belongs to the vehicle the panel was opened for; close
            // if that vehicle goes away or the active vehicle changes
            on_ActiveVehicleChanged: close()

            onClosed: destroy()

            sourceComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth / 2

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Cancel ROI")
                        onClicked: {
                            root._activeVehicle.stopGuidedModeROI()
                            roiEditDropPanel.close()
                        }
                    }

                    QGCButton {
                        Layout.fillWidth: true
                        text: qsTr("Edit Position")
                        onClicked: {
                            roiEditPositionDialogFactory.open()
                            roiEditDropPanel.close()
                        }
                    }
                }
            }
        }
    }
}
