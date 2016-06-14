/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1
import QtQuick.Layouts          1.2
import QtLocation               5.5
import QtPositioning            5.5

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

QGCView {
    id:             offlineMapView
    viewPanel:      panel
    anchors.fill:   parent

    property var    _currentSelection:  null

    property string mapKey:             "lastMapType"

    property string mapType:            QGroundControl.flightMapSettings.mapProvider + " " + QGroundControl.flightMapSettings.mapType
    property bool   isMapInteractive:   false
    property var    savedCenter:        undefined
    property real   savedZoom:          3
    property string savedMapType:       ""
    property bool   _showPreview:       true

    property bool _saveRealEstate:          ScreenTools.isTinyScreen || ScreenTools.isShortScreen
    property real _adjustableFontPointSize: _saveRealEstate ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize

    property var _mapAdjustedColor: _map.isSatelliteMap ? "white" : "black"

    readonly property real minZoomLevel: 3
    readonly property real maxZoomLevel: 20

    readonly property int _maxTilesForDownload: 60000

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        QGroundControl.mapEngineManager.loadTileSets()
        updateMap()
        savedCenter = _map.toCoordinate(Qt.point(_map.width / 2, _map.height / 2))
    }

    Connections {
        target: QGroundControl.mapEngineManager
        onTileSetsChanged: {
            setName.text = QGroundControl.mapEngineManager.getUniqueName()
        }
        onErrorMessageChanged: {
            errorDialog.visible = true
        }
    }

    ExclusiveGroup { id: setGroup }

    function handleChanges() {
        if(isMapInteractive) {
            var xl = 0
            var yl = 0
            var xr = _map.width.toFixed(0) - 1  // Must be within boundaries of visible map
            var yr = _map.height.toFixed(0) - 1 // Must be within boundaries of visible map
            var c0 = _map.toCoordinate(Qt.point(xl, yl))
            var c1 = _map.toCoordinate(Qt.point(xr, yr))
            QGroundControl.mapEngineManager.updateForCurrentView(c0.longitude, c0.latitude, c1.longitude, c1.latitude, sliderMinZoom.value, sliderMaxZoom.value, mapType)
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
        isMapInteractive = true
        mapType = QGroundControl.flightMapSettings.mapProvider + " " + QGroundControl.flightMapSettings.mapType
        resetMapToDefaults()
        handleChanges()
        _map.visible = true
        _tileSetList.visible = false
        infoView.visible = false
        defaultInfoView.visible = false
        addNewSetView.visible = true
    }

    function showList() {
        isMapInteractive = false
        _map.visible = false
        _tileSetList.visible = true
        infoView.visible = false
        defaultInfoView.visible = false
        addNewSetView.visible = false
    }

    function showInfo() {
        isMapInteractive = false
        if(_currentSelection && !offlineMapView._currentSelection.deleting) {
            enterInfoView()
        } else
            showList()
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

    function enterInfoView() {
        var isDefaultSet = offlineMapView._currentSelection.defaultSet
        _map.visible = true
        isMapInteractive = false
        savedCenter = _map.toCoordinate(Qt.point(_map.width / 2, _map.height / 2))
        savedZoom = _map.zoomLevel
        savedMapType = mapType
        if(!isDefaultSet) {
            mapType = offlineMapView._currentSelection.mapTypeStr
            _map.center = midPoint(offlineMapView._currentSelection.topleftLat, offlineMapView._currentSelection.bottomRightLat, offlineMapView._currentSelection.topleftLon, offlineMapView._currentSelection.bottomRightLon)
            //-- Delineate Set Region
            var x0 = offlineMapView._currentSelection.topleftLon
            var x1 = offlineMapView._currentSelection.bottomRightLon
            var y0 = offlineMapView._currentSelection.topleftLat
            var y1 = offlineMapView._currentSelection.bottomRightLat
            mapBoundary.topLeft     = QtPositioning.coordinate(y0, x0)
            mapBoundary.bottomRight = QtPositioning.coordinate(y1, x1)
            mapBoundary.visible = true
            _map.fitViewportToMapItems()
        }
        _tileSetList.visible = false
        addNewSetView.visible = false
        if(isDefaultSet) {
            defaultInfoView.visible = true
        } else {
            infoView.visible = true
        }
    }

    function leaveInfoView() {
        mapBoundary.visible = false
        _map.center = savedCenter
        _map.zoomLevel = savedZoom
        mapType = savedMapType
    }

    function resetMapToDefaults() {
        _map.center = QGroundControl.flightMapPosition
        _map.zoomLevel = QGroundControl.flightMapZoom
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    onMapTypeChanged: {
        updateMap()
        if(isMapInteractive) {
            QGroundControl.mapEngineManager.saveSetting(mapKey, mapType)
        }
    }

    MessageDialog {
        id:         errorDialog
        visible:    false
        text:       QGroundControl.mapEngineManager.errorMessage
        icon:       StandardIcon.Critical
        standardButtons: StandardButton.Ok
        title:      qsTr("Error Message")
        onYes: {
            errorDialog.visible = false
        }
    }

    Component {
        id: optionsDialogComponent

        QGCViewDialog {
            id: optionDialog

            function accept() {
                QGroundControl.mapEngineManager.mapboxToken  = mapBoxToken.text
                QGroundControl.mapEngineManager.maxDiskCache = parseInt(maxCacheSize.text)
                QGroundControl.mapEngineManager.maxMemCache  = parseInt(maxCacheMemSize.text)
                optionDialog.hideDialog()
            }

            QGCFlickable {
                anchors.fill:   parent
                contentHeight:  optionsColumn.height

                Column {
                    id:                 optionsColumn
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    QGCLabel { text:       qsTr("Max Cache Disk Size (MB):") }

                    QGCTextField {
                        id:                 maxCacheSize
                        maximumLength:      6
                        inputMethodHints:   Qt.ImhDigitsOnly
                        validator:          IntValidator {bottom: 1; top: 262144;}
                        text:               QGroundControl.mapEngineManager.maxDiskCache
                    }

                    Item { width: 1; height: 1 }

                    QGCLabel { text:       qsTr("Max Cache Memory Size (MB):") }

                    QGCTextField {
                        id:                 maxCacheMemSize
                        maximumLength:      4
                        inputMethodHints:   Qt.ImhDigitsOnly
                        validator:          IntValidator {bottom: 1; top: 4096;}
                        text:               QGroundControl.mapEngineManager.maxMemCache
                    }

                    QGCLabel {
                        font.pointSize: _adjustableFontPointSize
                        text:           qsTr("Memory cache changes require a restart to take effect.")
                    }

                    Item { width: 1; height: 1 }

                    QGCLabel { text: qsTr("MapBox Access Token") }

                    QGCTextField {
                        id:             mapBoxToken
                        maximumLength:  256
                        width:          ScreenTools.defaultFontPixelWidth * 30
                        text:           QGroundControl.mapEngineManager.mapboxToken
                    }

                    QGCLabel {
                        text:           qsTr("With an access token, you can use MapBox Maps.")
                        font.pointSize: _adjustableFontPointSize
                    }
                } // GridLayout
            } // QGCFlickable
        } // QGCViewDialog - optionsDialog
    } // Component - optionsDialogComponent

    Component {
        id: deleteConfirmationDialogComponent

        QGCViewMessage {
            id:         deleteConfirmationDialog
            message:    qsTr("Delete %1 and all its tiles.\n\nIs this really what you want?").arg(offlineMapView._currentSelection.name)

            function accept() {
                QGroundControl.mapEngineManager.deleteTileSet(offlineMapView._currentSelection)
                deleteConfirmationDialog.hideDialog()
                leaveInfoView()
                showList()
            }
        }
    }

    Component {
        id: deleteSystemSetConfirmationDialogComponent

        QGCViewMessage {
            id:         deleteSystemSetConfirmationDialog
            message:    qsTr("This will delete all tiles INCLUDING the tile sets you have created yourself.\n\nIs this really what you want?")

            function accept() {
                QGroundControl.mapEngineManager.deleteTileSet(offlineMapView._currentSelection)
                deleteSystemSetConfirmationDialog.hideDialog()
                leaveInfoView()
                showList()
            }
        }
    }

    QGCViewPanel {
        id:                 panel
        anchors.fill:       parent

        Map {
            id:                 _map
            anchors.fill:       parent
            center:             QGroundControl.lastKnownHomePosition
            visible:            false
            gesture.flickDeceleration:  3000

            property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

            plugin: Plugin { name: "QGroundControl" }

            MapRectangle {
                id:             mapBoundary
                border.width:   2
                border.color:   "red"
                color:          Qt.rgba(1,0,0,0.05)
                smooth:         true
                antialiasing:   true
            }

            Component.onCompleted: resetMapToDefaults()

            onCenterChanged:    handleChanges()
            onZoomLevelChanged: handleChanges()
            onWidthChanged:     handleChanges()
            onHeightChanged:    handleChanges()

            // Used to make pinch zoom work
            MouseArea {
                anchors.fill: parent
            }

            MapScale {
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                anchors.bottomMargin:   anchors.leftMargin
                anchors.left:           parent.left
                anchors.bottom:         parent.bottom
                mapControl:             _map
            }

            //-- Show Set Info
            Rectangle {
                id:                 infoView
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                y:                  Math.max(anchors.margins, (parent.height - (anchors.margins * 2) - height) / 2)
                anchors.right:      parent.right
                width:              Math.max(ScreenTools.defaultFontPixelWidth  * 20, controlInfoFlickable.width + (infoView._margins * 2))
                height:             Math.min(parent.height - (anchors.margins * 2), controlInfoFlickable.y + controlInfoColumn.height + ScreenTools.defaultFontPixelHeight)
                color:              qgcPal.window
                opacity:            0.85
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                visible:            false

                readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

                QGCLabel {
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 4
                    anchors.top:        parent.top
                    anchors.right:      parent.right
                    text:               "X"
                }

                Column {
                    id:                 titleColumn
                    anchors.margins:    infoView._margins
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           offlineMapView._currentSelection ? offlineMapView._currentSelection.name : ""
                        font.pointSize: _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                        horizontalAlignment: Text.AlignHCenter
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           offlineMapView._currentSelection ? offlineMapView._currentSelection.description : ""
                        visible:        text !== qsTr("Description")
                        horizontalAlignment: Text.AlignHCenter
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           offlineMapView._currentSelection ? "(" + offlineMapView._currentSelection.mapTypeStr + ")" : ""
                        horizontalAlignment: Text.AlignHCenter
                    }

                }

                MouseArea {
                    anchors.fill:       titleColumn
                    preventStealing:    true

                    onClicked: {
                        leaveInfoView()
                        showList()
                    }
                }

                QGCFlickable {
                    id:                 controlInfoFlickable
                    anchors.margins:    infoView._margins
                    anchors.top:        titleColumn.bottom
                    anchors.bottom:     parent.bottom
                    anchors.left:       parent.left
                    width:              controlInfoColumn.width
                    clip:               true
                    contentHeight:      controlInfoColumn.height

                    Column {
                        id:         controlInfoColumn
                        spacing:    ScreenTools.defaultFontPixelHeight

                        GridLayout {
                            columns:    2
                            rowSpacing: 0

                            QGCLabel { text: qsTr("Min Zoom:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.minZoom : "" }

                            QGCLabel { text: qsTr("Max Zoom:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.maxZoom : "" }

                            QGCLabel { text: qsTr("Total:") }
                            QGCLabel { text: (offlineMapView._currentSelection ? offlineMapView._currentSelection.numTilesStr : "") + " (" + (offlineMapView._currentSelection ? offlineMapView._currentSelection.tilesSizeStr : "") + ")" }

                            QGCLabel {
                                text:       qsTr("Downloaded:")
                                visible:    offlineMapView._currentSelection && !offlineMapView._currentSelection.complete
                            }
                            QGCLabel {
                                text:       (offlineMapView._currentSelection ? offlineMapView._currentSelection.savedTilesStr : "") + " (" + (offlineMapView._currentSelection ? offlineMapView._currentSelection.savedSizeStr : "") + ")"
                                visible:    offlineMapView._currentSelection && !offlineMapView._currentSelection.complete
                            }

                            QGCLabel {
                                text:       qsTr("Error Count:")
                                visible:    offlineMapView._currentSelection && !offlineMapView._currentSelection.complete
                            }
                            QGCLabel {
                                text:       offlineMapView._currentSelection ? offlineMapView._currentSelection.errorCountStr : ""
                                visible:    offlineMapView._currentSelection && !offlineMapView._currentSelection.complete
                            }
                        }

                        QGCButton {
                            text:       qsTr("Resume Download")
                            visible:    offlineMapView._currentSelection && (!offlineMapView._currentSelection.complete && !offlineMapView._currentSelection.downloading)

                            onClicked: {
                                if(offlineMapView._currentSelection)
                                    offlineMapView._currentSelection.resumeDownloadTask()
                            }
                        }

                        QGCButton {
                            text:       qsTr("Cancel Download")
                            visible:    offlineMapView._currentSelection && (!offlineMapView._currentSelection.complete && offlineMapView._currentSelection.downloading)

                            onClicked: {
                                if(offlineMapView._currentSelection)
                                    offlineMapView._currentSelection.cancelDownloadTask()
                            }
                        }

                        QGCButton {
                            text:       qsTr("Delete")
                            onClicked:  showDialog(deleteConfirmationDialogComponent, qsTr("Confirm Delete"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                        }
                    } // Column
                } // QGCFlickable
            } // Rectangle - infoView

            //-- Show Default Set Info
            Rectangle {
                id:                 defaultInfoView
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                y:                  Math.max(anchors.margins, (parent.height - (anchors.margins * 2) - height) / 2)
                anchors.right:      parent.right
                width:              ScreenTools.defaultFontPixelWidth  * 20
                height:             Math.min(parent.height - (anchors.margins * 2), defaultControlInfoFlickable.y + defaultControlInfoColumn.height + ScreenTools.defaultFontPixelHeight)
                color:              qgcPal.window
                opacity:            0.85
                radius:             ScreenTools.defaultFontPixelWidth * 0.5
                visible:            false

                QGCLabel {
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 4
                    anchors.top:        parent.top
                    anchors.right:      parent.right
                    text:               "X"
                }

                Column {
                    id:                 defaultTitleColumn
                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    anchors.right:      parent.right

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           offlineMapView._currentSelection ? offlineMapView._currentSelection.name : ""
                        font.pointSize: _saveRealEstate ? ScreenTools.defaultFontPointSize : ScreenTools.mediumFontPointSize
                        horizontalAlignment: Text.AlignHCenter
                    }

                    QGCLabel {
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        wrapMode:       Text.WordWrap
                        text:           qsTr("System Wide Tile Cache")
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                MouseArea {
                    anchors.fill:       defaultTitleColumn
                    preventStealing:    true

                    onClicked: {
                        leaveInfoView()
                        showList()
                    }
                }

                QGCFlickable {
                    id:                 defaultControlInfoFlickable
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    anchors.top:        defaultTitleColumn.bottom
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.bottom:     parent.bottom
                    clip:               true
                    contentHeight:      defaultControlInfoColumn.height

                    Column {
                        id:                 defaultControlInfoColumn
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing:            ScreenTools.defaultFontPixelHeight

                        GridLayout {
                            columns:    2
                            rowSpacing: 0

                            QGCLabel {
                                Layout.columnSpan:  2
                                text:               qsTr("System Cache")
                            }

                            QGCLabel { text: qsTr("Size:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.tilesSizeStr : "" }

                            QGCLabel { text: qsTr("Tile Count:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.numTilesStr : "" }

                            Item {
                                width:              1
                                height:             ScreenTools.defaultFontPixelHeight
                                Layout.columnSpan:  2
                            }

                            QGCLabel {
                                Layout.columnSpan:  2
                                text:               qsTr("All Sets")
                            }

                            QGCLabel { text: qsTr("Size:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.savedSizeStr : "" }

                            QGCLabel { text: qsTr("Tile Count:") }
                            QGCLabel { text: offlineMapView._currentSelection ? offlineMapView._currentSelection.savedTilesStr : ""}
                        }

                        QGCButton {
                            text:       qsTr("Delete All")
                            onClicked:  showDialog(deleteSystemSetConfirmationDialogComponent, qsTr("Confirm Delete All"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                        }
                    } // Column
                } // QGCFlickable
            } // Rectangle - defaultInfoView

            Item {
                id:             addNewSetView
                anchors.fill:   parent
                visible:        false

                Map {
                    id:                 minZoomPreview
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth /2
                    anchors.topMargin:  anchors.leftMargin
                    anchors.top:        parent.top
                    anchors.left:       parent.left
                    width:              parent.width / 4
                    height:             parent.height / 4
                    center:             _map.center
                    activeMapType:      _map.activeMapType
                    zoomLevel:          sliderMinZoom.value
                    gesture.enabled:    false
                    visible:            _showPreview

                    property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

                    plugin: Plugin { name: "QGroundControl" }

                    MapScale {
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                        anchors.bottomMargin:   anchors.leftMargin
                        anchors.left:           parent.left
                        anchors.bottom:         parent.bottom
                        mapControl:             parent
                    }
                }

                Map {
                    id:                 maxZoomPreview
                    anchors.topMargin:  minZoomPreview.anchors.topMargin
                    anchors.left:       minZoomPreview.left
                    anchors.top:        minZoomPreview.bottom
                    width:              minZoomPreview.width
                    height:             minZoomPreview.height
                    center:             _map.center
                    activeMapType:      _map.activeMapType
                    zoomLevel:          sliderMaxZoom.value
                    gesture.enabled:    false
                    visible:            _showPreview

                    property bool isSatelliteMap: activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1

                    plugin: Plugin { name: "QGroundControl" }

                    MapScale {
                        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth / 2
                        anchors.bottomMargin:   anchors.leftMargin
                        anchors.left:           parent.left
                        anchors.bottom:         parent.bottom
                        mapControl:             parent
                    }
                }

                Rectangle {
                    anchors.fill:   minZoomPreview
                    border.color:   _mapAdjustedColor
                    color:          "transparent"
                    visible:        _showPreview

                    QGCLabel {
                        anchors.fill:           parent
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        color:                  _mapAdjustedColor
                        text:                   qsTr("Min Zoom: %1").arg(sliderMinZoom.value)
                    }
                }

                Rectangle {
                    anchors.fill:   maxZoomPreview
                    border.color:   _mapAdjustedColor
                    color:          "transparent"
                    visible:        _showPreview

                    QGCLabel {
                        anchors.fill:           parent
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        color:                  _mapAdjustedColor
                        text:                   qsTr("Max Zoom: %1").arg(sliderMaxZoom.value)
                    }
                }

                MouseArea {
                    anchors.fill:   minZoomPreview
                    visible:        _showPreview
                    onClicked:      _showPreview = false
                }

                MouseArea {
                    anchors.fill:   maxZoomPreview
                    visible:        _showPreview
                    onClicked:      _showPreview = false
                }

                QGCButton {
                    anchors.left:   minZoomPreview.left
                    anchors.top:    minZoomPreview.top
                    text:           "Show zoom previews"
                    visible:        !_showPreview
                    onClicked:      _showPreview = !_showPreview
                }

                //-- Add new set dialog
                Rectangle {
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    y:                  Math.max(anchors.margins, (parent.height - (anchors.margins * 2) - height) / 2)
                    anchors.right:      parent.right
                    width:              ScreenTools.defaultFontPixelWidth  * 20
                    height:             Math.min(parent.height - (anchors.margins * 2), addNewSetFlickable.y + addNewSetColumn.height + ScreenTools.defaultFontPixelHeight)
                    color:              qgcPal.window
                    opacity:            0.85
                    radius:             ScreenTools.defaultFontPixelWidth * 0.5

                    QGCLabel {
                        anchors.margins:    ScreenTools.defaultFontPixelHeight / 4
                        anchors.top:        parent.top
                        anchors.right:      parent.right
                        text:               "X"
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

                    MouseArea {
                        anchors.fill:       addNewSetLabel
                        preventStealing:    true
                        onClicked:          showList()
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
                            spacing:            ScreenTools.defaultFontPixelHeight / (ScreenTools.isTinyScreen ? 4 : 2)

                            Column {
                                anchors.left:       parent.left
                                anchors.right:      parent.right

                                QGCLabel { text: qsTr("Name:") }

                                QGCTextField {
                                    id:             setName
                                    anchors.left:   parent.left
                                    anchors.right:  parent.right
                                }
                            }

                            Column {
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

                                    onActivated: {
                                        mapType = textAt(index)
                                        if(_dropButtonsExclusiveGroup.current)
                                            _dropButtonsExclusiveGroup.current.checked = false
                                        _dropButtonsExclusiveGroup.current = null
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
                            }

                            Rectangle {
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                height:         zoomColumn.height + ScreenTools.defaultFontPixelHeight / 2
                                color:          qgcPal.window
                                border.color:   qgcPal.text
                                radius:         ScreenTools.defaultFontPixelWidth * 0.5

                                Column {
                                    id:                 zoomColumn
                                    anchors.margins:    ScreenTools.defaultFontPixelHeight / 4
                                    anchors.top:        parent.top
                                    anchors.left:       parent.left
                                    anchors.right:      parent.right

                                    QGCLabel {
                                        text:           qsTr("Min Zoom: %1").arg(sliderMinZoom.value)
                                        font.pointSize: _adjustableFontPointSize
                                    }

                                    Slider {
                                        id:                         sliderMinZoom
                                        anchors.left:               parent.left
                                        anchors.right:              parent.right
                                        height:                     setName.height
                                        minimumValue:               minZoomLevel
                                        maximumValue:               maxZoomLevel
                                        stepSize:                   1
                                        updateValueWhileDragging:   true

                                        property real _savedZoom

                                        Component.onCompleted: sliderMinZoom.value = _map.zoomLevel - 2

                                        onValueChanged: {
                                            if(sliderMinZoom.value > sliderMaxZoom.value) {
                                                sliderMaxZoom.value = sliderMinZoom.value
                                            }
                                            handleChanges()
                                        }
                                    } // Slider - min zoom

                                    QGCLabel {
                                        text:                   qsTr("Max Zoom: %1").arg(sliderMaxZoom.value)
                                        font.pointSize:         _adjustableFontPointSize
                                    }

                                    Slider {
                                        id:                         sliderMaxZoom
                                        anchors.left:               parent.left
                                        anchors.right:              parent.right
                                        height:                     setName.height
                                        minimumValue:               minZoomLevel
                                        maximumValue:               maxZoomLevel
                                        stepSize:                   1
                                        updateValueWhileDragging:   true

                                        property real _savedZoom

                                        Component.onCompleted: sliderMaxZoom.value = _map.zoomLevel + 2

                                        onValueChanged: {
                                            if(sliderMaxZoom.value < sliderMinZoom.value) {
                                                sliderMinZoom.value = sliderMaxZoom.value
                                            }
                                            handleChanges()
                                        }
                                    } // Slider - max zoom

                                    GridLayout {
                                        columns:    2
                                        rowSpacing: 0

                                        QGCLabel {
                                            text:           qsTr("Tile Count")
                                            font.pointSize: _adjustableFontPointSize
                                        }
                                        QGCLabel {
                                            text:           QGroundControl.mapEngineManager.tileCountStr
                                            font.pointSize: _adjustableFontPointSize
                                        }

                                        QGCLabel {
                                            text:           qsTr("Set Size (Est)")
                                            font.pointSize: _adjustableFontPointSize
                                        }
                                        QGCLabel {
                                            text:           QGroundControl.mapEngineManager.tileSizeStr
                                            font.pointSize: _adjustableFontPointSize
                                        }
                                    }
                                } // Column - Zoom info
                            } // Rectangle - Zoom info

                            QGCButton {
                                text:       _tooManyTiles ? qsTr("Too many tiles") : qsTr("Download")
                                enabled:    !_tooManyTiles && setName.text.length > 0
                                anchors.horizontalCenter: parent.horizontalCenter

                                property bool _tooManyTiles: QGroundControl.mapEngineManager.tileCount > _maxTilesForDownload

                                onClicked: {
                                    if(QGroundControl.mapEngineManager.findName(setName.text)) {
                                        duplicateName.visible = true
                                    } else {
                                        /* This does not work if hosted by QQuickWidget. Waiting until we're 100% QtQuick
                                    var mapImage
                                    _map.grabToImage(function(result) { mapImage = result; })
                                    QGroundControl.mapEngineManager.startDownload(setName.text, "Description", mapType, mapImage);
                                    */
                                        QGroundControl.mapEngineManager.startDownload(setName.text, "Description" /* Description */, mapType);
                                        showList()
                                    }
                                }
                            }
                        } // Column
                    } // QGCFlickable
                } // Rectangle - Add new set dialog
            } // Item - Add new set view
        } // Map

        QGCFlickable {
            id:                 _tileSetList
            clip:               true
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            anchors.top:        parent.top
            anchors.bottom:     _optionsButton.top
            anchors.left:       parent.left
            anchors.right:      parent.right
            contentHeight:      _cacheList.height

            Column {
                id:         _cacheList
                width:      Math.min(_tileSetList.width, (ScreenTools.defaultFontPixelWidth  * 50).toFixed(0))
                spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                anchors.horizontalCenter: parent.horizontalCenter

                OfflineMapButton {
                    id:             firstButton
                    text:           qsTr("Add new set")
                    width:          _cacheList.width
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    onClicked: {
                        offlineMapView._currentSelection = null
                        addNewSet()
                    }
                }
                Repeater {
                    model: QGroundControl.mapEngineManager.tileSets
                    delegate: OfflineMapButton {
                        text:           object.name
                        size:           object.downloadStatus
                        complete:       object.complete
                        width:          firstButton.width
                        height:         ScreenTools.defaultFontPixelHeight * 2
                        onClicked: {
                            offlineMapView._currentSelection = object
                            showInfo()
                        }
                    }
                }
            }
        }

        QGCButton {
            id:              _optionsButton
            text:            qsTr("Options")
            visible:         _tileSetList.visible
            anchors.bottom:  parent.bottom
            anchors.right:   parent.right
            anchors.margins: ScreenTools.defaultFontPixelWidth
            onClicked:       showDialog(optionsDialogComponent, qsTr("Offline Maps Options"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
        }
    } // QGCViewPanel
} // QGCView
