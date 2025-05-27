/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FlightMap
import QGroundControl.QGCMapEngineManager
import QGroundControl.FactSystem
import QGroundControl.FactControls

FlightMap {
    id:                         _map
    allowGCSLocationCenter:     true
    allowVehicleLocationCenter: false
    mapName:                    "OfflineMap"

    property var tileSet:  null

    property string mapKey:             "lastMapType"

    property var    _settingsManager:   QGroundControl.settingsManager
    property var    _settings:          _settingsManager ? _settingsManager.offlineMapsSettings : null
    property var    _fmSettings:        _settingsManager ? _settingsManager.flightMapSettings : null
    property var    _appSettings:       _settingsManager.appSettings
    property Fact   _mapboxFact:        _settingsManager ? _settingsManager.appSettings.mapboxToken : null
    property Fact   _mapboxAccountFact: _settingsManager ? _settingsManager.appSettings.mapboxAccount : null
    property Fact   _mapboxStyleFact:   _settingsManager ? _settingsManager.appSettings.mapboxStyle : null
    property Fact   _esriFact:          _settingsManager ? _settingsManager.appSettings.esriToken : null
    property Fact   _customURLFact:     _settingsManager ? _settingsManager.appSettings.customURL : null
    property Fact   _vworldFact:        _settingsManager ? _settingsManager.appSettings.vworldToken : null

    property string mapType:            _fmSettings ? (_fmSettings.mapProvider.value + " " + _fmSettings.mapType.value) : ""
    property bool   isMapInteractive:   false
    property var    savedCenter:        undefined
    property real   savedZoom:          3
    property string savedMapType:       ""
    property bool   _showPreview:       true
    property bool   _defaultSet:        tileSet && tileSet.defaultSet
    property real   _margins:           ScreenTools.defaultFontPixelWidth * 0.5
    property real   _buttonSize:        ScreenTools.defaultFontPixelWidth * 12
    property real   _bigButtonSize:     ScreenTools.defaultFontPixelWidth * 16

    property bool   _saveRealEstate:          ScreenTools.isTinyScreen || ScreenTools.isShortScreen
    property real   _adjustableFontPointSize: _saveRealEstate ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize

    property var    _mapAdjustedColor:  _map.isSatelliteMap ? "white" : "black"
    property bool   _tooManyTiles:      QGroundControl.mapEngineManager.tileCount > _maxTilesForDownload
    property var    _addNewSetViewObject: null

    readonly property real minZoomLevel:    1
    readonly property real maxZoomLevel:    20
    readonly property real sliderTouchArea: ScreenTools.defaultFontPixelWidth * (ScreenTools.isTinyScreen ? 5 : (ScreenTools.isMobile ? 6 : 3))

    readonly property int _maxTilesForDownload: _settings ? _settings.maxTilesForDownload.rawValue : 0

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        QGroundControl.mapEngineManager.loadTileSets()
        resetMapToDefaults()
        updateMap()
        savedCenter = _map.toCoordinate(Qt.point(_map.width / 2, _map.height / 2), false /* clipToViewPort */)
        settingsPage.enabled = false // Prevent mouse events from bleeding through to the settings page which is below this in hierarchy
    }

    Component.onDestruction: settingsPage.enabled = true

    Connections {
        target:                 QGroundControl.mapEngineManager
        onErrorMessageChanged:  errorDialogComponent.createObject(mainWindow).open()
    }

    function handleChanges() {
        if (isMapInteractive) {
            var xl = 0
            var yl = 0
            var xr = _map.width.toFixed(0) - 1  // Must be within boundaries of visible map
            var yr = _map.height.toFixed(0) - 1 // Must be within boundaries of visible map
            var c0 = _map.toCoordinate(Qt.point(xl, yl), false /* clipToViewPort */)
            var c1 = _map.toCoordinate(Qt.point(xr, yr), false /* clipToViewPort */)
            QGroundControl.mapEngineManager.updateForCurrentView(c0.longitude, c0.latitude, c1.longitude, c1.latitude, _addNewSetViewObject.sliderMinZoom.value, _addNewSetViewObject.sliderMaxZoom.value, mapType)
        }
    }

    function updateMap() {
        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (mapType === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                handleChanges()
                return
            }
        }
    }

    function addNewSet() {
        _addNewSetViewObject = addNewSetViewComponent.createObject(_map)
        isMapInteractive = true
        mapType = _fmSettings.mapProvider.value + " " + _fmSettings.mapType.value
        resetMapToDefaults()
        handleChanges()
    }

    function showInfo() {
        isMapInteractive = false
        savedCenter = _map.toCoordinate(Qt.point(_map.width / 2, _map.height / 2), false /* clipToViewPort */)
        savedZoom = _map.zoomLevel
        savedMapType = mapType
        if (!tileSet.defaultSet) {
            mapType = tileSet.mapTypeStr
            _map.center = midPoint(tileSet.topleftLat, tileSet.bottomRightLat, tileSet.topleftLon, tileSet.bottomRightLon)
            //-- Delineate Set Region
            var x0 = tileSet.topleftLon
            var x1 = tileSet.bottomRightLon
            var y0 = tileSet.topleftLat
            var y1 = tileSet.bottomRightLat
            mapBoundary.topLeft     = QtPositioning.coordinate(y0, x0)
            mapBoundary.bottomRight = QtPositioning.coordinate(y1, x1)
            mapBoundary.visible = true
            // Some times, for whatever reason, the bounding box is correct (around ETH for instance), but the rectangle is drawn across the planet.
            // When that happens, the "_map.fitViewportToMapItems()" below makes the map to zoom to the entire earth.
            //console.log("Map boundary: " + mapBoundary.topLeft + " " + mapBoundary.bottomRight)
            _map.fitViewportToVisibleMapItems()
        }
        infoViewComponent.createObject(_map)
    }

    function toRadian(deg) {
        return deg * Math.PI / 180
    }

    function toDegree(rad) {
        return rad * 180 / Math.PI
    }

    function midPoint(lat1, lat2, lon1, lon2) {
        var dLon = toRadian(lon2 - lon1);
        lat1 = toRadian(lat1);
        lat2 = toRadian(lat2);
        lon1 = toRadian(lon1);
        var Bx = Math.cos(lat2) * Math.cos(dLon);
        var By = Math.cos(lat2) * Math.sin(dLon);
        var lat3 = Math.atan2(Math.sin(lat1) + Math.sin(lat2), Math.sqrt((Math.cos(lat1) + Bx) * (Math.cos(lat1) + Bx) + By * By));
        var lon3 = lon1 + Math.atan2(By, Math.cos(lat1) + Bx);
        return QtPositioning.coordinate(toDegree(lat3), toDegree(lon3))
    }

    function resetMapToDefaults() {
        _map.center = QGroundControl.flightMapPosition
        _map.zoomLevel = QGroundControl.flightMapZoom
    }

    onMapTypeChanged: {
        updateMap()
        if(isMapInteractive) {
            QGroundControl.mapEngineManager.saveSetting(mapKey, mapType)
        }
    }

    property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

    MapRectangle {
        id:             mapBoundary
        border.width:   2
        border.color:   "red"
        color:          Qt.rgba(1,0,0,0.05)
        smooth:         true
        antialiasing:   true
    }

    onCenterChanged:    handleChanges()
    onZoomLevelChanged: handleChanges()
    onWidthChanged:     handleChanges()
    onHeightChanged:    handleChanges()

    MapScale {
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
        anchors.bottomMargin:   anchors.leftMargin
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        mapControl:             _map
        buttonsOnLeft:          true
    }

    //-----------------------------------------------------------------
    //-- Show Set Info
    Component {
        id: infoViewComponent

        Rectangle {
            id:                 infoView
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.right:      parent.right
            anchors.verticalCenter: parent.verticalCenter
            width:              tileInfoColumn.width  + (ScreenTools.defaultFontPixelWidth  * 2)
            height:             tileInfoColumn.height + (ScreenTools.defaultFontPixelHeight * 2)
            color:              Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.85)
            radius:             ScreenTools.defaultFontPixelWidth * 0.5

            property bool       _extraButton: {
                if(!tileSet)
                    return false;
                var curSel = tileSet;
                return !_defaultSet && ((!curSel.complete && !curSel.downloading) || (!curSel.complete && curSel.downloading));
            }

            property real       _labelWidth:    ScreenTools.defaultFontPixelWidth * 10
            property real       _valueWidth:    ScreenTools.defaultFontPixelWidth * 14
            Column {
                id:                 tileInfoColumn
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.centerIn:   parent
                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           tileSet ? tileSet.name : ""
                    font.pointSize: _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    horizontalAlignment: Text.AlignHCenter
                    visible:        _defaultSet
                }
                QGCTextField {
                    id:             editSetName
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    visible:        !_defaultSet
                    text:           tileSet ? tileSet.name : ""
                }
                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text: {
                        if(tileSet) {
                            if(tileSet.defaultSet)
                                return qsTr("System Wide Tile Cache");
                            else
                                return "(" + tileSet.mapTypeStr + ")"
                        } else
                            return "";
                    }
                    horizontalAlignment: Text.AlignHCenter
                }
                //-- Tile Sets
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    !_defaultSet && mapType !== QGroundControl.elevationProviderName
                    QGCLabel {  text: qsTr("Zoom Levels:"); width: infoView._labelWidth; }
                    QGCLabel {  text: tileSet ? (tileSet.minZoom + " - " + tileSet.maxZoom) : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    !_defaultSet
                    QGCLabel {  text: qsTr("Total:"); width: infoView._labelWidth; }
                    QGCLabel {  text: (tileSet ? tileSet.totalTileCountStr : "") + " (" + (tileSet ? tileSet.totalTilesSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    tileSet && !_defaultSet && tileSet.uniqueTileCount > 0
                    QGCLabel {  text: qsTr("Unique:"); width: infoView._labelWidth; }
                    QGCLabel {  text: (tileSet ? tileSet.uniqueTileCountStr : "") + " (" + (tileSet ? tileSet.uniqueTileSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }

                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    tileSet && !_defaultSet && !tileSet.complete
                    QGCLabel {  text: qsTr("Downloaded:"); width: infoView._labelWidth; }
                    QGCLabel {  text: (tileSet ? tileSet.savedTileCountStr : "") + " (" + (tileSet ? tileSet.savedTileSizeStr : "") + ")"; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    tileSet && !_defaultSet && !tileSet.complete && tileSet.errorCount > 0
                    QGCLabel {  text: qsTr("Error Count:"); width: infoView._labelWidth; }
                    QGCLabel {  text: tileSet ? tileSet.errorCountStr : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                //-- Default Tile Set
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    _defaultSet
                    QGCLabel { text: qsTr("Size:"); width: infoView._labelWidth; }
                    QGCLabel { text: tileSet ? tileSet.savedTileSizeStr  : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible:    _defaultSet
                    QGCLabel { text: qsTr("Tile Count:"); width: infoView._labelWidth; }
                    QGCLabel { text: tileSet ? tileSet.savedTileCountStr : ""; horizontalAlignment: Text.AlignRight; width: infoView._valueWidth; }
                }
                Row {
                    spacing:    ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCButton {
                        text:       qsTr("Resume Download")
                        visible:    tileSet && tileSet && !_defaultSet && (!tileSet.complete && !tileSet.downloading)
                        width:      ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            if(tileSet)
                                tileSet.resumeDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Cancel Download")
                        visible:    tileSet && tileSet && !_defaultSet && (!tileSet.complete && tileSet.downloading)
                        width:      ScreenTools.defaultFontPixelWidth * 16
                        onClicked: {
                            if(tileSet)
                                tileSet.cancelDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Delete")
                        width:      ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        onClicked:  deleteConfirmationDialogComponent.createObject(mainWindow).open()
                        enabled:    tileSet ? (tileSet.savedTileSize > 0) : false
                    }
                    QGCButton {
                        text:       qsTr("Ok")
                        width:      ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        visible:    !_defaultSet
                        enabled:    editSetName.text !== ""
                        onClicked: {
                            if (editSetName.text !== tileSet.name) {
                                QGroundControl.mapEngineManager.renameTileSet(tileSet, editSetName.text)
                            }
                            _map.destroy()
                        }
                    }
                    QGCButton {
                        text:       _defaultSet ? qsTr("Close") : qsTr("Cancel")
                        width:      ScreenTools.defaultFontPixelWidth * (infoView._extraButton ? 6 : 10)
                        onClicked:  _map.destroy()
                    }
                }
            }
        }
    }

    Component {
        id: addNewSetViewComponent

        Item {
            id:             addNewSetView
            anchors.fill:   parent

            property var sliderMinZoom: sliderMinZoom
            property var sliderMaxZoom: sliderMaxZoom

            Column {
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin:     _margins
                anchors.left:           parent.left
                spacing:                _margins

                QGCButton {
                    text:       qsTr("Show zoom previews")
                    visible:    !_showPreview
                    onClicked:  _showPreview = !_showPreview
                }

                Map {
                    id:                 minZoomPreview
                    width:              addNewSetView.width / 4
                    height:             addNewSetView.height / 4
                    center:             _map.center
                    activeMapType:      _map.activeMapType
                    zoomLevel:          sliderMinZoom.value
                    visible:            _showPreview

                    property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

                    plugin: Plugin { name: "QGroundControl" }

                    MapScale {
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                        anchors.bottomMargin:   anchors.leftMargin
                        anchors.left:           parent.left
                        anchors.bottom:         parent.bottom
                        mapControl:             parent
                        zoomButtonsVisible:     false
                    }

                    Rectangle {
                        anchors.fill:   parent
                        border.color:   _mapAdjustedColor
                        color:          "transparent"

                        QGCMapLabel {
                            anchors.centerIn:   parent
                            map:                minZoomPreview
                            text:               qsTr("Min Zoom: %1").arg(sliderMinZoom.value)
                        }
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      _showPreview = false
                        }
                    }
                } // Map

                Map {
                    id:                 maxZoomPreview
                    width:              minZoomPreview.width
                    height:             minZoomPreview.height
                    center:             _map.center
                    activeMapType:      _map.activeMapType
                    zoomLevel:          sliderMaxZoom.value
                    visible:            _showPreview

                    property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

                    plugin: Plugin { name: "QGroundControl" }

                    MapScale {
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                        anchors.bottomMargin:   anchors.leftMargin
                        anchors.left:           parent.left
                        anchors.bottom:         parent.bottom
                        mapControl:             parent
                        zoomButtonsVisible:     false
                    }

                    Rectangle {
                        anchors.fill:   parent
                        border.color:   _mapAdjustedColor
                        color:          "transparent"

                        QGCMapLabel {
                            anchors.centerIn:   parent
                            map:                maxZoomPreview
                            text:               qsTr("Max Zoom: %1").arg(sliderMaxZoom.value)
                        }
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      _showPreview = false
                        }
                    }
                } // Map
            }

            Rectangle {
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.verticalCenter: parent.verticalCenter
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelWidth * (ScreenTools.isTinyScreen ? 24 : 28)
                height:             Math.min(parent.height - (anchors.margins * 2), addNewSetFlickable.y + addNewSetColumn.height + addNewSetLabel.anchors.margins)
                color:              Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.85)
                radius:             ScreenTools.defaultFontPixelWidth * 0.5

                //-- Eat mouse events
                DeadMouseArea {
                    anchors.fill: parent
                }

                QGCLabel {
                    id:                 addNewSetLabel
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    wrapMode:           Text.WordWrap
                    text:               qsTr("Add New Set")
                    font.pointSize:     _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                    horizontalAlignment: Text.AlignHCenter
                }

                QGCFlickable {
                    id:                     addNewSetFlickable
                    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                    anchors.rightMargin:    anchors.leftMargin
                    anchors.topMargin:      ScreenTools.defaultFontPixelWidth / 3
                    anchors.bottomMargin:   anchors.topMargin
                    anchors.top:            addNewSetLabel.bottom
                    anchors.left:           parent.left
                    anchors.right:          parent.right
                    anchors.bottom:         parent.bottom
                    clip:                   true
                    contentHeight:          addNewSetColumn.height

                    Column {
                        id:                 addNewSetColumn
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing:            ScreenTools.defaultFontPixelHeight * (ScreenTools.isTinyScreen ? 0.25 : 0.5)

                        Column {
                            spacing:            ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.25
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            QGCLabel { text: qsTr("Name:") }
                            QGCTextField {
                                id:                     setName
                                anchors.left:           parent.left
                                anchors.right:          parent.right
                                Component.onCompleted:  text = QGroundControl.mapEngineManager.getUniqueName()
                                Connections {
                                    target:             QGroundControl.mapEngineManager
                                    onTileSetsChanged:  setName.text = QGroundControl.mapEngineManager.getUniqueName()
                                }
                            }
                        }

                        Column {
                            spacing:            ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.25
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            QGCLabel {
                                text:       qsTr("Map type:")
                                visible:    !_saveRealEstate
                            }
                            QGCComboBox {
                                id:             mapCombo
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                model:          QGroundControl.mapEngineManager.mapList
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
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                text:           qsTr("Fetch elevation data")
                                checked:        QGroundControl.mapEngineManager.fetchElevation
                                onClicked: {
                                    QGroundControl.mapEngineManager.fetchElevation = checked
                                    handleChanges()
                                }
                            }
                        }

                        Rectangle {
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            height:         zoomColumn.height + ScreenTools.defaultFontPixelHeight * 0.5
                            color:          qgcPal.window
                            border.color:   qgcPal.text
                            radius:         ScreenTools.defaultFontPixelWidth * 0.5

                            Column {
                                id:                 zoomColumn
                                spacing:            ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.5
                                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.25
                                anchors.top:        parent.top
                                anchors.left:       parent.left
                                anchors.right:      parent.right

                                QGCLabel {
                                    text:           qsTr("Min/Max Zoom Levels")
                                    font.pointSize: _adjustableFontPointSize
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }

                                Slider {
                                    id:                         sliderMinZoom
                                    anchors.left:               parent.left
                                    anchors.right:              parent.right
                                    height:                     sliderTouchArea * 1.25
                                    from:                       minZoomLevel
                                    to:                         maxZoomLevel
                                    stepSize:                   1
                                    live:                       true
                                    property bool _updateSetting: false
                                    Component.onCompleted: {
                                        sliderMinZoom.value = _settings.minZoomLevelDownload.rawValue
                                        _updateSetting = true
                                    }
                                    onValueChanged: {
                                        if(sliderMinZoom.value > sliderMaxZoom.value) {
                                            sliderMaxZoom.value = sliderMinZoom.value
                                        }
                                        if (_updateSetting) {
                                            // Don't update setting until after Component.onCompleted since bad values come through before that
                                            _settings.minZoomLevelDownload.rawValue = value
                                        }
                                        handleChanges()
                                    }
                                    handle: Rectangle {
                                        x: sliderMinZoom.leftPadding + sliderMinZoom.visualPosition  * (sliderMinZoom.availableWidth - width)
                                        y: sliderMinZoom.topPadding  + sliderMinZoom.availableHeight * 0.5 - height * 0.5
                                        implicitWidth:  sliderTouchArea
                                        implicitHeight: sliderTouchArea
                                        radius:         sliderTouchArea * 0.5
                                        color:          qgcPal.button
                                        border.width:   1
                                        border.color:   qgcPal.buttonText
                                        Label {
                                            text:               sliderMinZoom.value
                                            anchors.centerIn:   parent
                                            font.family:        ScreenTools.normalFontFamily
                                            font.pointSize:     ScreenTools.smallFontPointSize
                                            color:              qgcPal.buttonText
                                        }
                                    }
                                } // Slider - min zoom

                                Slider {
                                    id:                         sliderMaxZoom
                                    anchors.left:               parent.left
                                    anchors.right:              parent.right
                                    height:                     sliderTouchArea * 1.25
                                    from:                       minZoomLevel
                                    to:                         maxZoomLevel
                                    stepSize:                   1
                                    live:                       true
                                    property bool _updateSetting: false
                                    Component.onCompleted: {
                                        sliderMaxZoom.value = _settings.maxZoomLevelDownload.rawValue
                                        _updateSetting = true
                                    }
                                    onValueChanged: {
                                        if(sliderMaxZoom.value < sliderMinZoom.value) {
                                            sliderMinZoom.value = sliderMaxZoom.value
                                        }
                                        if (_updateSetting) {
                                            // Don't update setting until after Component.onCompleted since bad values come through before that
                                            _settings.maxZoomLevelDownload.rawValue = value
                                        }
                                        handleChanges()
                                    }
                                    handle: Rectangle {
                                        x: sliderMaxZoom.leftPadding + sliderMaxZoom.visualPosition  * (sliderMaxZoom.availableWidth - width)
                                        y: sliderMaxZoom.topPadding  + sliderMaxZoom.availableHeight * 0.5 - height * 0.5
                                        implicitWidth:  sliderTouchArea
                                        implicitHeight: sliderTouchArea
                                        radius:         sliderTouchArea * 0.5
                                        color:          qgcPal.button
                                        border.width:   1
                                        border.color:   qgcPal.buttonText
                                        Label {
                                            text:               sliderMaxZoom.value
                                            anchors.centerIn:   parent
                                            font.family:        ScreenTools.normalFontFamily
                                            font.pointSize:     ScreenTools.smallFontPointSize
                                            color:              qgcPal.buttonText
                                        }
                                    }
                                } // Slider - max zoom

                                GridLayout {
                                    columns:    2
                                    rowSpacing: ScreenTools.isTinyScreen ? 0 : ScreenTools.defaultFontPixelHeight * 0.5
                                    QGCLabel {
                                        text:           qsTr("Tile Count:")
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                    QGCLabel {
                                        text:            QGroundControl.mapEngineManager.tileCountStr
                                        font.pointSize: _adjustableFontPointSize
                                    }

                                    QGCLabel {
                                        text:           qsTr("Est Size:")
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                    QGCLabel {
                                        text:           QGroundControl.mapEngineManager.tileSizeStr
                                        font.pointSize: _adjustableFontPointSize
                                    }
                                }
                            } // Column - Zoom info
                        } // Rectangle - Zoom info

                        QGCLabel {
                            text:       qsTr("Too many tiles")
                            visible:    _tooManyTiles
                            color:      qgcPal.warningText
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Row {
                            id: addButtonRow
                            spacing: ScreenTools.defaultFontPixelWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            QGCButton {
                                text:       qsTr("Download")
                                width:      (addNewSetColumn.width * 0.5) - (addButtonRow.spacing * 0.5)
                                enabled:    !_tooManyTiles && setName.text.length > 0
                                onClicked: {
                                    if (QGroundControl.mapEngineManager.findName(setName.text)) {
                                        duplicateName.visible = true
                                    } else {
                                        QGroundControl.mapEngineManager.startDownload(setName.text, mapType);
                                        _map.destroy()
                                    }
                                }
                            }
                            QGCButton {
                                text:       qsTr("Cancel")
                                width:      (addNewSetColumn.width * 0.5) - (addButtonRow.spacing * 0.5)
                                onClicked:  _map.destroy()
                            }
                        }
                    }
                }
            } 
        }
    }

    CenterMapDropButton {
        id:                 centerMapButton
        topMargin:          0
        anchors.margins:    _margins
        anchors.left:       map.left
        anchors.top:        map.top
        map:                _map
        showMission:        false
        showAllItems:       false
        visible:            _addNewSetViewObject
    }

    Component {
        id: errorDialogComponent

        QGCSimpleMessageDialog {
            title:      qsTr("Error Message")
            text:       _mapEngineManager.errorMessage
            buttons:    Dialog.Close
        }
    }

    Component {
        id: deleteConfirmationDialogComponent

        QGCSimpleMessageDialog {
            title:      qsTr("Confirm Delete")
            text:       tileSet.defaultSet ?
                            qsTr("This will delete all tiles INCLUDING the tile sets you have created yourself.\n\nIs this really what you want?") :
                            qsTr("Delete %1 and all its tiles.\n\nIs this really what you want?").arg(tileSet.name)
            buttons:    Dialog.Yes | Dialog.No

            onAccepted: {
                QGroundControl.mapEngineManager.deleteTileSet(tileSet)
                _map.destroy()
            }
        }
    }
}

