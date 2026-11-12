import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap
import QGroundControl.GeoMap

/// Editor of an offline map set, over a map that fills it: the area of a new set is what that map shows.
/// The map is a QtLocation FlightMap or a GeoMapView. The editor uses what both offer (center, zoomLevel,
/// toCoordinate, isSatelliteMap, showMapType, ...) and one function each engine's map adds:
/// showSetArea(topLeft, bottomRight).
Item {
    id: _editor

    property var tileSet: null

    property var _map: _mapLoader.item
    property bool _useGeoMap: false

    property var _settingsManager: QGroundControl.settingsManager
    property var _settings: _settingsManager.offlineMapsSettings

    property string mapType: QGroundControl.mapEngineManager.chosenMap
    property bool isMapInteractive: false
    property bool _showPreview: true
    property bool _defaultSet: tileSet && tileSet.defaultSet
    property real _margins: ScreenTools.defaultFontPixelWidth * 0.5

    property bool _saveRealEstate: ScreenTools.isTinyScreen || ScreenTools.isShortScreen
    property real _adjustableFontPointSize: _saveRealEstate ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize

    property string _mapAdjustedColor: _map && _map.isSatelliteMap ? "white" : "black"
    property bool _tooManyTiles: QGroundControl.mapEngineManager.tileCount > _maxTilesForDownload
    property var _addNewSetViewObject: null

    readonly property real minZoomLevel: 1
    readonly property real maxZoomLevel: 20
    readonly property real sliderTouchArea: ScreenTools.defaultFontPixelWidth * (ScreenTools.isTinyScreen ? 5 : (ScreenTools.isMobile ? 6 : 3))

    readonly property int _maxTilesForDownload: _settings.maxTilesForDownload.rawValue

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        // The engine is chosen when the editor opens, as the Fly View chooses its own at startup
        _useGeoMap = _settingsManager.flyViewSettings.useGeoMapEngine.rawValue
        _mapLoader.sourceComponent = _useGeoMap ? geoMapComponent : flightMapComponent
        resetMapToDefaults()
        updateMap()
    }

    Connections {
        target: QGroundControl.mapEngineManager
        function onErrorMessageChanged() { errorDialogFactory.open() }
    }

    function handleChanges() {
        if (isMapInteractive) {
            const xr = Math.round(_map.width) - 1  // Must be within boundaries of visible map
            const yr = Math.round(_map.height) - 1 // Must be within boundaries of visible map
            const c0 = _map.toCoordinate(Qt.point(0, 0), false /* clipToViewPort */)
            const c1 = _map.toCoordinate(Qt.point(xr, yr), false /* clipToViewPort */)
            if (!c0.isValid || !c1.isValid) {
                // The map has no size yet
                return
            }
            QGroundControl.mapEngineManager.updateForCurrentView(c0.longitude, c0.latitude, c1.longitude, c1.latitude, _addNewSetViewObject.sliderMinZoom.value, _addNewSetViewObject.sliderMaxZoom.value, mapType)
        }
    }

    function updateMap() {
        // The map type is first set before the map is created
        if (_map) {
            // The map keeps what it shows for a type it cannot show, such as an elevation provider. The tile
            // estimate of a new set follows the type all the same.
            _map.showMapType(mapType)
            handleChanges()
        }
    }

    function addNewSet() {
        _addNewSetViewObject = addNewSetViewComponent.createObject(_editor)
        isMapInteractive = true
        mapType = QGroundControl.mapEngineManager.chosenMap
        resetMapToDefaults()
        handleChanges()
    }

    function showInfo() {
        isMapInteractive = false
        if (!tileSet.defaultSet) {
            mapType = tileSet.mapTypeStr
            //-- Delineate Set Region
            _map.showSetArea(QtPositioning.coordinate(tileSet.topleftLat, tileSet.topleftLon),
                             QtPositioning.coordinate(tileSet.bottomRightLat, tileSet.bottomRightLon))
        }
        infoViewComponent.createObject(_editor)
    }

    function resetMapToDefaults() {
        _map.center = QGroundControl.flightMapPosition
        _map.zoomLevel = QGroundControl.flightMapZoom
    }

    onMapTypeChanged: updateMap()

    // Under the map: a GeoMap takes only the gestures it handles, and the rest must not reach the settings page
    // that the editor covers. Its drags can still be taken over by the map.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onWheel: (wheel) => wheel.accepted = true
    }

    Loader {
        id: _mapLoader
        anchors.fill: parent
    }

    Component {
        id: flightMapComponent

        FlightMap {
            function showSetArea(topLeft, bottomRight) {
                mapBoundary.topLeft = topLeft
                mapBoundary.bottomRight = bottomRight
                mapBoundary.visible = true
                // The fit below takes each corner where it is drawn: on the side of the map's center where it is
                // nearer. From a center half a world away the corners can be on both sides, and it fits the world.
                center = QtPositioning.rectangle(topLeft, bottomRight).center
                // Some times, for whatever reason, the bounding box is correct (around ETH for instance), but the rectangle is drawn across the planet.
                // When that happens, the "fitViewportToVisibleMapItems()" below makes the map to zoom to the entire earth.
                fitViewportToVisibleMapItems()
            }

            objectName: "offlineMapEditor_flightMap"
            allowGCSLocationCenter: true
            allowVehicleLocationCenter: false
            mapName: "OfflineMap"

            MapRectangle {
                id: mapBoundary
                border.width: 2
                border.color: "red"
                color: Qt.rgba(1, 0, 0, 0.05)
                smooth: true
                antialiasing: true
                visible: false
            }
        }
    }

    Component {
        id: geoMapComponent

        // Flat, top-down and north-up like the QtLocation map: the area of a set is the rectangle of ground on screen.
        // Its view is its own: looking at a set elsewhere must not move the map of the Fly View.
        GeoMapView {
            id: geoMap

            function showSetArea(topLeft, bottomRight) {
                setArea.coordinates = [ topLeft, QtPositioning.coordinate(topLeft.latitude, bottomRight.longitude),
                                        bottomRight, QtPositioning.coordinate(bottomRight.latitude, topLeft.longitude) ]
                setArea.visible = true
                // Room around the area, so that its outline is not on the edge of the map
                const latitudeMargin = (topLeft.latitude - bottomRight.latitude) * 0.1
                const longitudeMargin = (bottomRight.longitude - topLeft.longitude) * 0.1
                setVisibleRegion(QtPositioning.rectangle(
                    QtPositioning.coordinate(Math.min(topLeft.latitude + latitudeMargin, 85), Math.max(topLeft.longitude - longitudeMargin, -180)),
                    QtPositioning.coordinate(Math.max(bottomRight.latitude - latitudeMargin, -85), Math.min(bottomRight.longitude + longitudeMargin, 180))))
            }

            objectName: "offlineMapEditor_geoMap"
            sharedView: false
            terrain: false
            allowRotation: false
            allowGCSLocationCenter: true
            allowVehicleLocationCenter: false
            followOffscreenVehicle: false

            GeoMapPolygon {
                id: setArea
                scene: geoMap.scene
                surfaceModel: geoMap.surfaceModel
                strokeColor: "red"
                strokeWidth: 2
                fillColor: Qt.rgba(1, 0, 0, 0.05)
                visible: false
            }
        }
    }

    Connections {
        target: _map
        function onCenterChanged() { handleChanges() }
        function onZoomLevelChanged() { handleChanges() }
        function onWidthChanged() { handleChanges() }
        function onHeightChanged() { handleChanges() }
    }

    // What lies on the map whatever its engine. Created once the map is, as it reads the map's properties.
    Loader {
        anchors.fill: parent
        active: _map !== null

        sourceComponent: Item {
            MapScale {
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                anchors.bottomMargin: anchors.leftMargin + _map.attributionHeight
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                mapControl: _map
            }

            CenterMapDropButton {
                topMargin: 0
                anchors.margins: _margins
                anchors.left: parent.left
                anchors.top: parent.top
                map: _map
                showMission: false
                showAllItems: false
                visible: _addNewSetViewObject
            }
        }
    }

    //-----------------------------------------------------------------
    //-- Show Set Info
    Component {
        id: infoViewComponent

        Rectangle {
            id: infoView
            anchors.margins: ScreenTools.defaultFontPixelHeight
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: tileInfoColumn.width + ScreenTools.defaultFontPixelWidth * 2
            height: tileInfoColumn.height + ScreenTools.defaultFontPixelHeight * 2
            color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.85)
            radius: ScreenTools.defaultFontPixelWidth * 0.5

            property bool _extraButton: !_defaultSet && tileSet && !tileSet.complete

            property real _labelWidth: ScreenTools.defaultFontPixelWidth * 10
            property real _valueWidth: ScreenTools.defaultFontPixelWidth * 14
            Column {
                id: tileInfoColumn
                spacing: ScreenTools.defaultFontPixelHeight * 0.5
                anchors.centerIn: parent
                QGCLabel {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    wrapMode: Text.WordWrap
                    text: tileSet ? tileSet.name : ""
                    font.pointSize: _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    horizontalAlignment: Text.AlignHCenter
                    visible: _defaultSet
                }
                QGCTextField {
                    id: editSetName
                    objectName: "offlineMapEditor_setNameField"
                    anchors.left: parent.left
                    anchors.right: parent.right
                    visible: !_defaultSet
                    text: tileSet ? tileSet.name : ""
                }
                QGCLabel {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    wrapMode: Text.WordWrap
                    text: {
                        if (tileSet) {
                            if (tileSet.defaultSet)
                                return qsTr("System Wide Tile Cache")
                            else
                                return "(" + tileSet.mapTypeStr + ")"
                        } else
                            return ""
                    }
                    horizontalAlignment: Text.AlignHCenter
                }
                //-- Tile Sets
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !_defaultSet && mapType !== QGroundControl.elevationProviderName
                    QGCLabel { text: qsTr("Zoom Levels:"); width: infoView._labelWidth }
                    QGCLabel { text: tileSet ? (tileSet.minZoom + " - " + tileSet.maxZoom) : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: !_defaultSet
                    QGCLabel { text: qsTr("Total:"); width: infoView._labelWidth }
                    QGCLabel { text: (tileSet ? tileSet.totalTileCountStr : "") + " (" + (tileSet ? tileSet.totalTilesSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: tileSet && !_defaultSet && tileSet.uniqueTileCount > 0
                    QGCLabel { text: qsTr("Unique:"); width: infoView._labelWidth }
                    QGCLabel { text: (tileSet ? tileSet.uniqueTileCountStr : "") + " (" + (tileSet ? tileSet.uniqueTileSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }

                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: tileSet && !_defaultSet && !tileSet.complete
                    QGCLabel { text: qsTr("Downloaded:"); width: infoView._labelWidth }
                    QGCLabel { text: (tileSet ? tileSet.savedTileCountStr : "") + " (" + (tileSet ? tileSet.savedTileSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: tileSet && !_defaultSet && !tileSet.complete && tileSet.errorCount > 0
                    QGCLabel { text: qsTr("Error Count:"); width: infoView._labelWidth }
                    QGCLabel { text: tileSet ? tileSet.errorCountStr : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                //-- Default Tile Set
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: _defaultSet
                    QGCLabel { text: qsTr("Size:"); width: infoView._labelWidth }
                    QGCLabel { text: tileSet ? tileSet.savedTileSizeStr : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: _defaultSet
                    QGCLabel { text: qsTr("Tile Count:"); width: infoView._labelWidth }
                    QGCLabel { text: tileSet ? tileSet.savedTileCountStr : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth }
                }
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCButton {
                        text: qsTr("Resume Download")
                        visible: tileSet && !_defaultSet && !tileSet.complete && !tileSet.downloading
                        width: ScreenTools.defaultFontPixelWidth * 16
                        onClicked: tileSet.resumeDownloadTask()
                    }
                    QGCButton {
                        text: qsTr("Cancel Download")
                        visible: tileSet && !_defaultSet && !tileSet.complete && tileSet.downloading
                        width: ScreenTools.defaultFontPixelWidth * 16
                        onClicked: tileSet.cancelDownloadTask()
                    }
                    QGCButton {
                        text: qsTr("Delete")
                        width: ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        onClicked: deleteConfirmationDialogFactory.open()
                        enabled: tileSet && tileSet.savedTileSize > 0
                    }
                    QGCButton {
                        objectName: "offlineMapEditor_okButton"
                        text: qsTr("Ok")
                        width: ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        visible: !_defaultSet
                        enabled: editSetName.text !== ""
                        onClicked: {
                            if (editSetName.text !== tileSet.name) {
                                QGroundControl.mapEngineManager.renameTileSet(tileSet, editSetName.text)
                            }
                            _editor.destroy()
                        }
                    }
                    QGCButton {
                        objectName: "offlineMapEditor_closeButton"
                        text: _defaultSet ? qsTr("Close") : qsTr("Cancel")
                        width: ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        onClicked: _editor.destroy()
                    }
                }
            }
        }
    }

    Component {
        id: addNewSetViewComponent

        Item {
            id: addNewSetView
            anchors.fill: parent

            property var sliderMinZoom: sliderMinZoom
            property var sliderMaxZoom: sliderMaxZoom

            Column {
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: _margins
                anchors.left: parent.left
                spacing: _margins

                QGCButton {
                    text: qsTr("Show zoom previews")
                    visible: !_showPreview
                    onClicked: _showPreview = !_showPreview
                }

                Repeater {
                    model: [
                        { slider: sliderMinZoom, label: qsTr("Min Zoom: %1") },
                        { slider: sliderMaxZoom, label: qsTr("Max Zoom: %1") }
                    ]

                    // What the map looks like at one end of the set's zoom range
                    Item {
                        id: zoomPreview
                        objectName: "offlineMapEditor_zoomPreview"
                        width: addNewSetView.width / 4
                        height: addNewSetView.height / 4
                        visible: _showPreview

                        required property var modelData
                        readonly property real zoomLevel: modelData.slider.value

                        Loader {
                            anchors.fill: parent
                            sourceComponent: _useGeoMap ? geoMapPreviewComponent : flightMapPreviewComponent
                        }

                        Component {
                            id: flightMapPreviewComponent

                            Map {
                                center: _map.center
                                activeMapType: _map.activeMapType
                                zoomLevel: zoomPreview.zoomLevel

                                property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

                                plugin: Plugin { name: "QGroundControl" }

                                MapScale {
                                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                                    anchors.bottomMargin: anchors.leftMargin
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    mapControl: parent
                                }
                            }
                        }

                        Component {
                            id: geoMapPreviewComponent

                            // A picture of the zoom level: it takes no gestures, the editor's map is the one to move
                            GeoMapView {
                                id: geoMapPreview
                                enabled: false
                                sharedView: false
                                terrain: false
                                allowRotation: false
                                showAttribution: false
                                mapType: _map.mapType

                                // Not bindings on center and zoomLevel themselves: GeoMapView assigns to them
                                Binding { target: geoMapPreview; property: "center"; value: _map.center }
                                Binding { target: geoMapPreview; property: "zoomLevel"; value: zoomPreview.zoomLevel }

                                MapScale {
                                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
                                    anchors.bottomMargin: anchors.leftMargin
                                    anchors.left: parent.left
                                    anchors.bottom: parent.bottom
                                    mapControl: geoMapPreview
                                }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            border.color: _mapAdjustedColor
                            color: "transparent"

                            QGCMapLabel {
                                anchors.centerIn: parent
                                map: _map
                                text: zoomPreview.modelData.label.arg(zoomPreview.zoomLevel)
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: _showPreview = false
                            }
                        }
                    }
                }
            }

            Rectangle {
                anchors.margins: ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                width: ScreenTools.defaultFontPixelWidth * (ScreenTools.isTinyScreen ? 24 : 28)
                height: Math.min(parent.height - (anchors.margins * 2), addNewSetFlickable.y + addNewSetColumn.height + addNewSetLabel.anchors.margins)
                color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.85)
                radius: ScreenTools.defaultFontPixelWidth * 0.5

                //-- Eat mouse events
                DeadMouseArea {
                    anchors.fill: parent
                }

                QGCLabel {
                    id: addNewSetLabel
                    anchors.margins: ScreenTools.defaultFontPixelHeight / 2
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    wrapMode: Text.WordWrap
                    text: qsTr("Add New Set")
                    font.pointSize: _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    horizontalAlignment: Text.AlignHCenter
                }

                QGCFlickable {
                    id: addNewSetFlickable
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                    anchors.rightMargin: anchors.leftMargin
                    anchors.topMargin: ScreenTools.defaultFontPixelWidth / 3
                    anchors.bottomMargin: anchors.topMargin
                    anchors.top: addNewSetLabel.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    clip: true
                    contentHeight: addNewSetColumn.height

                    Column {
                        id: addNewSetColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: ScreenTools.defaultFontPixelHeight * (ScreenTools.isTinyScreen ? 0.25 : 0.5)

                        Column {
                            spacing: ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.25
                            anchors.left: parent.left
                            anchors.right: parent.right
                            QGCLabel { text: qsTr("Name:") }
                            QGCTextField {
                                id: setName
                                anchors.left: parent.left
                                anchors.right: parent.right
                                Component.onCompleted: text = QGroundControl.mapEngineManager.getUniqueName()
                                onTextChanged: duplicateName.visible = false
                                Connections {
                                    target: QGroundControl.mapEngineManager
                                    function onTileSetsChanged() { setName.text = QGroundControl.mapEngineManager.getUniqueName() }
                                }
                            }
                        }

                        Column {
                            spacing: ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.25
                            anchors.left: parent.left
                            anchors.right: parent.right
                            QGCLabel {
                                text: qsTr("Map type:")
                                visible: !_saveRealEstate
                            }
                            QGCComboBox {
                                id: mapCombo
                                objectName: "offlineMapEditor_mapTypeCombo"
                                anchors.left: parent.left
                                anchors.right: parent.right
                                model: QGroundControl.mapEngineManager.mapList
                                onActivated: (index) => {
                                    mapType = textAt(index)
                                }
                                Component.onCompleted: {
                                    var index = mapCombo.find(mapType)
                                    if (index === -1) {
                                        console.warn("Active map name not in combo", mapType)
                                    } else {
                                        mapCombo.currentIndex = index
                                    }
                                }
                            }
                            QGCCheckBox {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                text: qsTr("Fetch elevation data")
                                checked: QGroundControl.mapEngineManager.fetchElevation
                                onClicked: {
                                    QGroundControl.mapEngineManager.fetchElevation = checked
                                    handleChanges()
                                }
                            }
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: zoomColumn.height + ScreenTools.defaultFontPixelHeight * 0.5
                            color: qgcPal.window
                            border.color: qgcPal.textPrimary
                            radius: ScreenTools.defaultFontPixelWidth * 0.5

                            Column {
                                id: zoomColumn
                                spacing: ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.5
                                anchors.margins: ScreenTools.defaultFontPixelHeight * 0.25
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.right: parent.right

                                QGCLabel {
                                    text: qsTr("Min/Max Zoom Levels")
                                    font.pointSize: _adjustableFontPointSize
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }

                                Slider {
                                    id: sliderMinZoom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    height: sliderTouchArea * 1.25
                                    from: minZoomLevel
                                    to: maxZoomLevel
                                    stepSize: 1
                                    live: true
                                    property bool _updateSetting: false
                                    Component.onCompleted: {
                                        sliderMinZoom.value = _settings.minZoomLevelDownload.rawValue
                                        _updateSetting = true
                                    }
                                    onValueChanged: {
                                        if (sliderMinZoom.value > sliderMaxZoom.value) {
                                            sliderMaxZoom.value = sliderMinZoom.value
                                        }
                                        if (_updateSetting) {
                                            // Don't update setting until after Component.onCompleted since bad values come through before that
                                            _settings.minZoomLevelDownload.rawValue = value
                                        }
                                        handleChanges()
                                    }
                                    handle: Rectangle {
                                        x: sliderMinZoom.leftPadding + sliderMinZoom.visualPosition * (sliderMinZoom.availableWidth - width)
                                        y: sliderMinZoom.topPadding + sliderMinZoom.availableHeight * 0.5 - height * 0.5
                                        implicitWidth: sliderTouchArea
                                        implicitHeight: sliderTouchArea
                                        radius: sliderTouchArea * 0.5
                                        color: qgcPal.windowCard
                                        border.width: 1
                                        border.color: qgcPal.textPrimary
                                        Label {
                                            text: sliderMinZoom.value
                                            anchors.centerIn: parent
                                            font.family: ScreenTools.normalFontFamily
                                            font.pointSize: ScreenTools.smallFontPointSize
                                            color: qgcPal.textPrimary
                                        }
                                    }
                                } // Slider - min zoom

                                Slider {
                                    id: sliderMaxZoom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    height: sliderTouchArea * 1.25
                                    from: minZoomLevel
                                    to: maxZoomLevel
                                    stepSize: 1
                                    live: true
                                    property bool _updateSetting: false
                                    Component.onCompleted: {
                                        sliderMaxZoom.value = _settings.maxZoomLevelDownload.rawValue
                                        _updateSetting = true
                                    }
                                    onValueChanged: {
                                        if (sliderMaxZoom.value < sliderMinZoom.value) {
                                            sliderMinZoom.value = sliderMaxZoom.value
                                        }
                                        if (_updateSetting) {
                                            // Don't update setting until after Component.onCompleted since bad values come through before that
                                            _settings.maxZoomLevelDownload.rawValue = value
                                        }
                                        handleChanges()
                                    }
                                    handle: Rectangle {
                                        x: sliderMaxZoom.leftPadding + sliderMaxZoom.visualPosition * (sliderMaxZoom.availableWidth - width)
                                        y: sliderMaxZoom.topPadding + sliderMaxZoom.availableHeight * 0.5 - height * 0.5
                                        implicitWidth: sliderTouchArea
                                        implicitHeight: sliderTouchArea
                                        radius: sliderTouchArea * 0.5
                                        color: qgcPal.windowCard
                                        border.width: 1
                                        border.color: qgcPal.textPrimary
                                        Label {
                                            text: sliderMaxZoom.value
                                            anchors.centerIn: parent
                                            font.family: ScreenTools.normalFontFamily
                                            font.pointSize: ScreenTools.smallFontPointSize
                                            color: qgcPal.textPrimary
                                        }
                                    }
                                } // Slider - max zoom

                                GridLayout {
                                    columns: 2
                                    rowSpacing: ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.5
                                    QGCLabel {
                                        text: qsTr("Tile Count:")
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                    QGCLabel {
                                        text: QGroundControl.mapEngineManager.tileCountStr
                                        font.pointSize: _adjustableFontPointSize
                                    }

                                    QGCLabel {
                                        text: qsTr("Est Size:")
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                    QGCLabel {
                                        text: QGroundControl.mapEngineManager.tileSizeStr
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                }
                            } // Column - Zoom info
                        } // Rectangle - Zoom info

                        QGCLabel {
                            text: qsTr("Too many tiles")
                            visible: _tooManyTiles
                            color: qgcPal.errorText
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        QGCLabel {
                            id: duplicateName
                            text: qsTr("Tile set with this name already exists")
                            visible: false
                            color: qgcPal.errorText
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Row {
                            id: addButtonRow
                            spacing: ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                text: qsTr("Download")
                                width: (addNewSetColumn.width * 0.5) - (addButtonRow.spacing * 0.5)
                                enabled: !_tooManyTiles && setName.text.length > 0
                                onClicked: {
                                    if (QGroundControl.mapEngineManager.findName(setName.text)) {
                                        duplicateName.visible = true
                                    } else {
                                        QGroundControl.mapEngineManager.startDownload(setName.text, mapType)
                                        _editor.destroy()
                                    }
                                }
                            }
                            QGCButton {
                                objectName: "offlineMapEditor_cancelButton"
                                text: qsTr("Cancel")
                                width: (addNewSetColumn.width * 0.5) - (addButtonRow.spacing * 0.5)
                                onClicked: _editor.destroy()
                            }
                        }
                    }
                }
            }
        }
    }

    QGCPopupDialogFactory {
        id: errorDialogFactory

        dialogComponent: errorDialogComponent
    }

    Component {
        id: errorDialogComponent

        QGCSimpleMessageDialog {
            title: qsTr("Error Message")
            text: QGroundControl.mapEngineManager.errorMessage
            buttons: Dialog.Close
        }
    }

    QGCPopupDialogFactory {
        id: deleteConfirmationDialogFactory

        dialogComponent: deleteConfirmationDialogComponent
    }

    Component {
        id: deleteConfirmationDialogComponent

        QGCSimpleMessageDialog {
            title: qsTr("Confirm Delete")
            text: tileSet.defaultSet ?
                            qsTr("This will delete all tiles INCLUDING the tile sets you have created yourself.\n\nIs this really what you want?") :
                            qsTr("Delete %1 and all its tiles.\n\nIs this really what you want?").arg(tileSet.name)
            buttons: Dialog.Yes | Dialog.No

            onAccepted: {
                QGroundControl.mapEngineManager.deleteTileSet(tileSet)
                _editor.destroy()
            }
        }
    }
}
