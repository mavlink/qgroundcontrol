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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:                 _offlineMapRoot
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _currentSelection:  null

    property string mapKey:             "lastMapType"

    property string mapType:            QGroundControl.mapEngineManager.loadSetting(mapKey, "Google Street Map")
    property int    mapMargin:          (ScreenTools.defaultFontPixelHeight * 0.2).toFixed(0)
    property real   infoWidth:          Math.max(infoCol.width, (ScreenTools.defaultFontPixelWidth * 40))
    property bool   isDefaultSet:       _offlineMapRoot._currentSelection && _offlineMapRoot._currentSelection.defaultSet
    property bool   isMapInteractive:   true
    property var    savedCenter:        undefined
    property real   savedZoom:          3
    property string savedMapType:       ""

    property real   _newSetMiddleLabel: ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelWidth * 10 : ScreenTools.defaultFontPixelWidth * 12
    property real   _newSetMiddleField: ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelWidth * 16 : ScreenTools.defaultFontPixelWidth * 20
    property real   _netSetSliderWidth: ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelWidth *  8 : ScreenTools.defaultFontPixelWidth * 16

    property real oldlon0:      0
    property real oldlon1:      0
    property real oldlat0:      0
    property real oldlat1:      0
    property int  oldz0:        0
    property int  oldz1:        0

    readonly property real minZoomLevel: 3
    readonly property real maxZoomLevel: 20

    QGCPalette { id: qgcpal }

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
            var xl = mapMargin
            var yl = mapMargin
            var xr = _map.width.toFixed(0)  - mapMargin
            var yr = _map.height.toFixed(0) - mapMargin
            var c0 = _map.toCoordinate(Qt.point(xl, yl))
            var c1 = _map.toCoordinate(Qt.point(xr, yr))
            if(oldlon0 !== c0.longitude || oldlat0 !== c0.latitude || oldlon1 !== c1.longitude || oldlat1 !== c1.latitude || oldz0 !== _slider0.value || oldz1 !== _slider1.value) {
                QGroundControl.mapEngineManager.updateForCurrentView(c0.longitude, c0.latitude, c1.longitude, c1.latitude, _slider0.value, _slider1.value, mapType)
            }
        }
    }

    function checkSanity() {
        if(isMapInteractive && QGroundControl.mapEngineManager.crazySize) {
            _slider1.value = _slider1.value - 1
            handleChanges()
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

    function showOptions() {
        _map.visible = false
        _tileSetList.visible = false
        _infoView.visible = false
        _defaultInfoView.visible = false
        _mapView.visible = false
        _optionsView.visible = true
    }

    function showMap() {
        _map.visible = true
        _tileSetList.visible = false
        _infoView.visible = false
        _defaultInfoView.visible = false
        _mapView.visible = true
        _optionsView.visible = false
    }

    function showList() {
        _map.visible = false
        _tileSetList.visible = true
        _infoView.visible = false
        _defaultInfoView.visible = false
        _mapView.visible = false
        _optionsView.visible = false
    }

    function showInfo() {
        if(_currentSelection && !_offlineMapRoot._currentSelection.deleting) {
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
        if(!isDefaultSet) {
            isMapInteractive = false
            savedCenter = _map.toCoordinate(Qt.point(_map.width / 2, _map.height / 2))
            savedZoom = _map.zoomLevel
            savedMapType = mapType
            _map.visible = true
            mapType = _offlineMapRoot._currentSelection.mapTypeStr
            _map.center = midPoint(_offlineMapRoot._currentSelection.topleftLat, _offlineMapRoot._currentSelection.bottomRightLat, _offlineMapRoot._currentSelection.topleftLon, _offlineMapRoot._currentSelection.bottomRightLon)
            //-- Delineate Set Region
            var x0 = _offlineMapRoot._currentSelection.topleftLon
            var x1 = _offlineMapRoot._currentSelection.bottomRightLon
            var y0 = _offlineMapRoot._currentSelection.topleftLat
            var y1 = _offlineMapRoot._currentSelection.bottomRightLat
            mapBoundary.topLeft     = QtPositioning.coordinate(y0, x0)
            mapBoundary.bottomRight = QtPositioning.coordinate(y1, x1)
            mapBoundary.visible = true
            _map.fitViewportToMapItems()
        }
        _tileSetList.visible = false
        _mapView.visible     = false
        _optionsView.visible = false
        if(isDefaultSet) {
            _defaultInfoView.visible = true
        } else {
            _infoView.visible= true
        }
    }

    function leaveInfoView() {
        mapBoundary.visible = false
        _map.center = savedCenter
        _map.zoomLevel = savedZoom
        mapType = savedMapType
        isMapInteractive = true
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
        title:      qsTr("Errror Message")
        onYes: {
            errorDialog.visible = false
        }
    }

    Rectangle {
        id:         _offlineMapTopRect
        width:      parent.width
        height:     labelTitle.height + ScreenTools.defaultFontPixelHeight
        color:      qgcPal.window
        anchors.top: parent.top
        Row {
            spacing: ScreenTools.defaultFontPixelHeight * 2
            anchors.verticalCenter: parent.verticalCenter
            QGCLabel {
                id:         labelTitle
                text:       qsTr("Offline Maps")
                font.pointSize: ScreenTools.mediumFontPointSize
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCCheckBox {
                id:         showTilePreview
                text:       qsTr("Show tile min/max zoom level preview")
                checked:    false
                visible:    _mapView.visible && !ScreenTools.isTinyScreen
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Map {
        id:                 _map
        anchors.top:        _offlineMapTopRect.bottom
        anchors.left:       parent.left
        anchors.bottom:     parent.bottom
        anchors.margins:    mapMargin
        width:              parent.width - ScreenTools.defaultFontPixelWidth
        center:             QGroundControl.defaultMapPosition
        visible:            false
        gesture.flickDeceleration:  3000
        plugin: Plugin { name: "QGroundControl" }

        Rectangle {
            color: Qt.rgba(0,0,0,0)
            border.color: "black"
            border.width: 1
            anchors.fill: parent
        }

        MapRectangle {
            id:             mapBoundary
            border.width:   2
            border.color:   "red"
            color:          Qt.rgba(1,0,0,0.05)
            smooth:         true
            antialiasing:   true
        }

        Component.onCompleted: {
            center = QGroundControl.flightMapPosition
            zoomLevel = QGroundControl.flightMapZoom
        }

        onCenterChanged: {
            handleChanges()
            checkSanity()
        }
        onZoomLevelChanged: {
            handleChanges()
            checkSanity()
        }
        onWidthChanged: {
            handleChanges()
            checkSanity()
        }
        onHeightChanged: {
            handleChanges()
            checkSanity()
        }
        // Used to make pinch zoom work
        MouseArea {
            anchors.fill: parent
        }
    }

    QGCFlickable {
        id:                 _tileSetList
        clip:               true
        anchors.top:        _offlineMapTopRect.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.bottom:     _optionsButton.top
        contentHeight:      _cacheList.height
        flickableDirection: Flickable.VerticalFlick

        Column {
            id:                 _cacheList
            width:              Math.min(parent.width, (ScreenTools.defaultFontPixelWidth  * 50).toFixed(0))
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.horizontalCenter: parent.horizontalCenter

            OfflineMapButton {
                text:           qsTr("Add new set")
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         ScreenTools.defaultFontPixelHeight * 2
                onClicked: {
                    _offlineMapRoot._currentSelection = null
                    showMap()
                }
            }
            Repeater {
                model: QGroundControl.mapEngineManager.tileSets
                delegate: OfflineMapButton {
                    text:           object.name
                    size:           object.downloadStatus
                    complete:       object.complete
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    onClicked: {
                        _offlineMapRoot._currentSelection = object
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
        onClicked:       showOptions()
    }

    //-- Offline Map Definition
    Item {
        id:                 _mapView
        width:              parent.width
        anchors.top:        _offlineMapTopRect.bottom
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        visible:            false

        //-- Zoom Preview Maps
        Item {
            width:          parent.width
            anchors.top:    parent.top
            visible:        showTilePreview.checked
            Rectangle {
                width:              ScreenTools.defaultFontPixelHeight * 16
                height:             ScreenTools.defaultFontPixelHeight * 9
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                color:              "black"
                Map {
                    id:                 _mapMin
                    anchors.fill:       parent
                    anchors.margins:    2
                    zoomLevel:          _slider0.value
                    center:             _map.center
                    gesture.enabled:    false
                    activeMapType:      _map.activeMapType
                    plugin: Plugin { name: "QGroundControl" }
                }
            }
            Rectangle {
                width:              ScreenTools.defaultFontPixelHeight * 16
                height:             ScreenTools.defaultFontPixelHeight * 9
                anchors.top:        parent.top
                anchors.right:      parent.right
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                color:              "black"
                Map {
                    id:                 _mapMax
                    anchors.fill:       parent
                    anchors.margins:    2
                    zoomLevel:          _slider1.value
                    center:             _map.center
                    gesture.enabled:    false
                    activeMapType:      _map.activeMapType
                    plugin: Plugin { name: "QGroundControl" }
                }
            }
        }
        //-- Tile set settings
        Rectangle {
            id:     bottomRect
            width:  _controlRow.width  + (ScreenTools.defaultFontPixelWidth  * 2)
            height: _controlRow.height + (ScreenTools.defaultFontPixelHeight * 2)
            color:  qgcPal.window
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            Component.onCompleted: {
                color = Qt.rgba(color.r, color.g, color.b, 0.85)
            }
            anchors.bottom: parent.bottom
            Row {
                id: _controlRow
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                Rectangle {
                    height:         _zoomRow.height + ScreenTools.defaultFontPixelHeight * 1.5
                    width:          _zoomRow.width  + ScreenTools.defaultFontPixelWidth
                    color:          "#98aca4"
                    border.color:   "black"
                    border.width:   2
                    radius:     ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    Row {
                        id:                 _zoomRow
                        anchors.centerIn:   parent
                        Column {
                            spacing:        ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.verticalCenter: parent.verticalCenter
                            Row {
                                spacing:    ScreenTools.defaultFontPixelWidth * 0.5
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        text:                   qsTr("Min")
                                        color:                  "black"
                                        width:                  ScreenTools.defaultFontPixelWidth * 4
                                        font.pointSize:         ScreenTools.smallFontPointSize
                                        horizontalAlignment:    Text.AlignHCenter
                                        font.family:            ScreenTools.normalFontFamily
                                    }
                                    Label {
                                        text:                   qsTr("Zoom")
                                        color:                  "black"
                                        width:                  ScreenTools.defaultFontPixelWidth * 4
                                        font.family:            ScreenTools.normalFontFamily
                                        font.pointSize:         ScreenTools.smallFontPointSize
                                        horizontalAlignment:    Text.AlignHCenter
                                    }
                                }
                                Slider {
                                    id:                 _slider0
                                    minimumValue:       minZoomLevel
                                    maximumValue:       maxZoomLevel
                                    stepSize:           1
                                    tickmarksEnabled:   false
                                    orientation:        Qt.Horizontal
                                    updateValueWhileDragging: true
                                    anchors.verticalCenter: parent.verticalCenter
                                    style: SliderStyle {
                                        groove: Rectangle {
                                            implicitWidth:  _netSetSliderWidth
                                            implicitHeight: 4
                                            color:          "gray"
                                            radius:         4
                                        }
                                        handle: Rectangle {
                                            anchors.centerIn: parent
                                            color: control.pressed ? "white" : "lightgray"
                                            border.color: "gray"
                                            border.width:   2
                                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 3
                                            implicitHeight: ScreenTools.defaultFontPixelWidth * 3
                                            radius:         10
                                            Label {
                                                text:               _slider0.value
                                                anchors.centerIn:   parent
                                                font.family:        ScreenTools.normalFontFamily
                                                font.pointSize:     ScreenTools.smallFontPointSize
                                            }
                                        }
                                    }
                                    Component.onCompleted: {
                                        _slider0.value = _map.zoomLevel - 2
                                    }
                                    onValueChanged: {
                                        if(_slider1) {
                                            if(_slider0.value > _slider1.value)
                                                _slider1.value = _slider0.value
                                            else {
                                                handleChanges()
                                                checkSanity()
                                            }
                                        }
                                    }
                                }
                            }
                            Row {
                                spacing:        ScreenTools.defaultFontPixelWidth * 0.5
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        text:           qsTr("Max")
                                        color:          "black"
                                        width:          ScreenTools.defaultFontPixelWidth * 4
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        font.family:    ScreenTools.normalFontFamily
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Label {
                                        text:           qsTr("Zoom")
                                        color:          "black"
                                        width:          ScreenTools.defaultFontPixelWidth * 4
                                        font.pointSize: ScreenTools.smallFontPointSize
                                        font.family:    ScreenTools.normalFontFamily
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                }
                                Slider {
                                    id:                 _slider1
                                    minimumValue:       minZoomLevel
                                    maximumValue:       maxZoomLevel
                                    stepSize:           1
                                    tickmarksEnabled:   false
                                    orientation:        Qt.Horizontal
                                    updateValueWhileDragging: true
                                    anchors.verticalCenter: parent.verticalCenter
                                    style: SliderStyle {
                                        groove: Rectangle {
                                            implicitWidth:  _netSetSliderWidth
                                            implicitHeight: 4
                                            color:          "gray"
                                            radius:         4
                                        }
                                        handle: Rectangle {
                                            anchors.centerIn: parent
                                            color: control.pressed ? "white" : "lightgray"
                                            border.color: "gray"
                                            border.width:   2
                                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 3
                                            implicitHeight: ScreenTools.defaultFontPixelWidth * 3
                                            radius:         10
                                            Label {
                                                text:               _slider1.value
                                                anchors.centerIn:   parent
                                                font.family:        ScreenTools.normalFontFamily
                                                font.pointSize:     ScreenTools.smallFontPointSize
                                            }
                                        }
                                    }
                                    Component.onCompleted: {
                                        _slider1.value = _map.zoomLevel + 2
                                    }
                                    onValueChanged: {
                                        if(_slider1.value < _slider0.value)
                                            _slider0.value = _slider1.value
                                        else {
                                            handleChanges()
                                            checkSanity()
                                        }
                                    }
                                }
                            }
                        }
                        Column {
                            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.verticalCenter: parent.verticalCenter
                            Label {
                                text:           qsTr("Tile Count")
                                color:          "black"
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                font.pointSize: ScreenTools.smallFontPointSize
                                font.family:    ScreenTools.normalFontFamily
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:           QGroundControl.mapEngineManager.tileCountStr
                                color:          "black"
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                font.family:    ScreenTools.normalFontFamily
                                font.pointSize: ScreenTools.defaultFontPointSize
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:           qsTr("Set Size (Est)")
                                color:          "black"
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                font.pointSize: ScreenTools.smallFontPointSize
                                font.family:    ScreenTools.normalFontFamily
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:           QGroundControl.mapEngineManager.tileSizeStr
                                color:          "black"
                                width:          ScreenTools.defaultFontPixelWidth * 12
                                font.family:    ScreenTools.normalFontFamily
                                font.pointSize: ScreenTools.defaultFontPointSize
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:   qsTr("Name:")
                            width:  _newSetMiddleLabel
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCTextField {
                            id:     setName
                            width:  _newSetMiddleField
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:   qsTr("Description:")
                            width:  _newSetMiddleLabel
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCTextField {
                            id:     setDescription
                            text:   qsTr("Description")
                            width:  _newSetMiddleField
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:   qsTr("Map Type:")
                            width:  _newSetMiddleLabel
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCComboBox {
                            id:         mapCombo
                            width:      _newSetMiddleField
                            model:      QGroundControl.mapEngineManager.mapList
                            onActivated: {
                                mapType = textAt(index)
                                if(_dropButtonsExclusiveGroup.current)
                                    _dropButtonsExclusiveGroup.current.checked = false
                                _dropButtonsExclusiveGroup.current = null
                            }
                            Component.onCompleted: {
                                var index = mapCombo.find(mapType)
                                if (index === -1) {
                                    console.warn(qsTr("Active map name not in combo"), mapType)
                                } else {
                                    mapCombo.currentIndex = index
                                }
                            }
                        }
                    }
                }
                Item {
                    height: 1
                    width:  ScreenTools.defaultFontPixelWidth
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: ScreenTools.defaultFontPixelHeight * 0.5
                    QGCButton {
                        text:  qsTr("Download")
                        enabled: setName.text.length > 0
                        width: ScreenTools.defaultFontPixelWidth * 10
                        onClicked: {
                            if(QGroundControl.mapEngineManager.findName(setName.text)) {
                                duplicateName.visible = true
                            } else {
                                /* This does not work if hosted by QQuickWidget. Waiting until we're 100% QtQuick
                                var mapImage
                                _map.grabToImage(function(result) { mapImage = result; })
                                QGroundControl.mapEngineManager.startDownload(setName.text, setDescription.text, mapType, mapImage);
                                */
                                QGroundControl.mapEngineManager.startDownload(setName.text, setDescription.text, mapType);
                                showList()
                            }
                        }
                    }
                    QGCButton {
                        text:  qsTr("Cancel")
                        width: ScreenTools.defaultFontPixelWidth * 10
                        onClicked: {
                            showList()
                        }
                    }
                    MessageDialog {
                        id:         duplicateName
                        visible:    false
                        icon:       StandardIcon.Warning
                        standardButtons: StandardButton.Ok
                        title:      qsTr("Tile Set Already Exists")
                        text:       qsTr("Tile Set \"%1\" already exists.\nPlease select a different name.").arg(setName.text)
                        onYes: {
                            duplicateName.visible = false
                        }
                    }
                }
            }
        }
    }

    //-- Show Set Info
    Item {
        id:                 _infoView
        width:              parent.width
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        visible:            false

        //-- Tile set settings
        Rectangle {
            id:     bottomInfoRect
            width:  _controlInfoRow.width  + (ScreenTools.defaultFontPixelWidth  * 2)
            height: _controlInfoRow.height + (ScreenTools.defaultFontPixelHeight * 2)
            color:  qgcPal.window
            radius: ScreenTools.defaultFontPixelWidth * 0.5
            anchors.margins: ScreenTools.defaultFontPixelWidth
            anchors.horizontalCenter: parent.horizontalCenter
            Component.onCompleted: {
                color = Qt.rgba(color.r, color.g, color.b, 0.85)
            }
            anchors.bottom: parent.bottom
            Row {
                id: _controlInfoRow
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth * 4
                Column {
                    anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
                    anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
                    anchors.verticalCenter: parent.verticalCenter
                    spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                    QGCLabel {
                        text:   _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.name : ""
                        font.pointSize:   ScreenTools.isAndroid ? ScreenTools.mediumFontPointSize : ScreenTools.largeFontPointSize
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:    _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.description : ""
                        visible: text !== qsTr("Description")
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    QGCLabel {
                        text:   _offlineMapRoot._currentSelection ? "(" + _offlineMapRoot._currentSelection.mapTypeStr + ")" : ""
                    }
                }
                GridLayout {
                    columns:            2
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    rowSpacing:         ScreenTools.defaultFontPixelWidth
                    columnSpacing:      ScreenTools.defaultFontPixelHeight
                    QGCLabel {
                        text:       qsTr("Min Zoom:")
                    }
                    QGCLabel {
                        text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.minZoom : ""
                    }
                    QGCLabel {
                        text:       qsTr("Max Zoom:")
                    }
                    QGCLabel {
                        text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.maxZoom : ""
                    }
                    QGCLabel {
                        text:       qsTr("Total:")
                    }
                    QGCLabel {
                        text:       (_offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.numTilesStr : "") + " (" + (_offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.tilesSizeStr : "") + ")"
                    }
                    QGCLabel {
                        text:       qsTr("Downloaded:")
                        visible:    _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                    }
                    QGCLabel {
                        text:        (_offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedTilesStr : "") + " (" + (_offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedSizeStr : "") + ")"
                        visible:    _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                    }
                    QGCLabel {
                        text:       qsTr("Error Count:")
                        visible:    _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                    }
                    QGCLabel {
                        text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.errorCountStr : ""
                        visible:    _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing:  ScreenTools.defaultFontPixelHeight * 0.5
                    QGCButton {
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        text:       qsTr("Delete")
                        enabled:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.deleting)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                deleteDialog.visible = true
                        }
                        MessageDialog {
                            id:         deleteDialog
                            visible:    false
                            icon:       StandardIcon.Warning
                            standardButtons: StandardButton.Yes | StandardButton.No
                            title:      qsTr("Delete Tile Set")
                            text:       {
                                if(_offlineMapRoot._currentSelection) {
                                    var blurb = qsTr("Delete %1 and all its tiles.\nIs this really what you want?").arg(_offlineMapRoot._currentSelection.name)
                                    return blurb
                                }
                                return ""
                            }
                            onYes: {
                                leaveInfoView()
                                if(_offlineMapRoot._currentSelection)
                                    QGroundControl.mapEngineManager.deleteTileSet(_offlineMapRoot._currentSelection)
                                deleteDialog.visible = false
                                showList()
                            }
                            onNo: {
                                deleteDialog.visible = false
                            }
                        }
                    }
                    QGCButton {
                        text:       qsTr("Resume Download")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        enabled:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.deleting && !_offlineMapRoot._currentSelection.downloading)
                        visible:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.complete && !_offlineMapRoot._currentSelection.downloading)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                _offlineMapRoot._currentSelection.resumeDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Cancel Download")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        enabled:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.deleting && _offlineMapRoot._currentSelection.downloading)
                        visible:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.complete && _offlineMapRoot._currentSelection.downloading)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                _offlineMapRoot._currentSelection.cancelDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Back")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked: {
                            leaveInfoView()
                            showList()
                        }
                    }
                }
            }
        }
    }

    //-- Show info on default tile set
    Rectangle {
        id:                 _defaultInfoView
        color:              qgcPal.windowShade
        width:              parent.width
        anchors.top:        _offlineMapTopRect.bottom
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        visible:            false
        QGCFlickable {
            id:                 infoScroll
            anchors.fill:       parent
            contentHeight:      infoColumn.height
            flickableDirection: Flickable.VerticalFlick
            clip:               true
            Column {
                id:             infoColumn
                width:          parent.width
                spacing:        ScreenTools.defaultFontPixelHeight
                Item {
                    height:     ScreenTools.defaultFontPixelHeight * 0.5
                    width:      1
                }
                Rectangle {
                    id:         _infoNameRect
                    width:      infoWidth
                    height:     infoCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color:      qgcPal.window
                    radius:     ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    Column {
                        id:         infoCol
                        spacing:    ScreenTools.defaultFontPixelHeight
                        anchors.centerIn: parent
                        QGCLabel {
                            id:     nameLabel
                            text:   _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.name : ""
                            font.pointSize:   ScreenTools.isAndroid ? ScreenTools.mediumFontPointSize : ScreenTools.largeFontPointSize
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCLabel {
                            id:     descLabel
                            text:   _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.description : ""
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
                Rectangle {
                    id:         _infoRect
                    width:      infoWidth
                    height:     infoGrid.height + (ScreenTools.defaultFontPixelHeight * 4)
                    color:      qgcPal.window
                    radius:     ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    GridLayout {
                        id:                 infoGrid
                        columns:            2
                        anchors.centerIn:   parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth  * 2
                        rowSpacing:         ScreenTools.defaultFontPixelWidth
                        columnSpacing:      ScreenTools.defaultFontPixelHeight * 2
                        QGCLabel {
                            text:       qsTr("Default Set Size:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.tilesSizeStr : ""
                        }
                        QGCLabel {
                            text:       qsTr("Default Set Tile Count:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.numTilesStr : ""
                        }
                        QGCLabel {
                            text:       qsTr("Total Size (All Sets):")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedSizeStr : ""
                        }
                        QGCLabel {
                            text:       qsTr("Total Count (All Sets):")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedTilesStr : ""
                        }
                    }
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: ScreenTools.defaultFontPixelWidth
                    QGCButton {
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        text:       qsTr("Delete")
                        enabled:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.deleting)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                deleteDefaultDialog.visible = true
                        }
                        MessageDialog {
                            id:         deleteDefaultDialog
                            visible:    false
                            icon:       StandardIcon.Warning
                            standardButtons: StandardButton.Yes | StandardButton.No
                            title:      qsTr("Delete All Tiles")
                            text:       qsTr("Delete all cached tiles.\nIs this really what you want?")
                            onYes: {
                                if(_offlineMapRoot._currentSelection)
                                    QGroundControl.mapEngineManager.deleteTileSet(_offlineMapRoot._currentSelection)
                                deleteDefaultDialog.visible = false
                                showList()
                            }
                            onNo: {
                                deleteDefaultDialog.visible = false
                            }
                        }
                    }
                    QGCButton {
                        text:       qsTr("Back")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked: {
                            showList()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id:                 _optionsView
        color:              qgcPal.windowShade
        width:              parent.width
        anchors.top:        _offlineMapTopRect.bottom
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        visible:            false
        onVisibleChanged: {
            if(_optionsView.visible) {
                mapBoxToken.text     = QGroundControl.mapEngineManager.mapboxToken
                maxCacheSize.text    = QGroundControl.mapEngineManager.maxDiskCache
                maxCacheMemSize.text = QGroundControl.mapEngineManager.maxMemCache
            }
        }
        QGCFlickable {
            id:                 optionsScroll
            anchors.fill:       parent
            contentHeight:      optionsColumn.height
            flickableDirection: Flickable.VerticalFlick
            clip:               true
            Column {
                id:             optionsColumn
                width:          parent.width
                spacing:        ScreenTools.defaultFontPixelHeight
                Item {
                    height:     ScreenTools.defaultFontPixelHeight
                    width:      1
                }
                Rectangle {
                    width:      infoWidth
                    height:     optionsLabel.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color:      qgcPal.window
                    radius:     ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:     optionsLabel
                        text:   qsTr("Offline Map Options")
                        font.pointSize:     ScreenTools.largeFontPointSize
                        anchors.centerIn:   parent
                    }
                }
                Rectangle {
                    id:         optionsRect
                    width:      optionsGrid.width  + (ScreenTools.defaultFontPixelWidth  * 4)
                    height:     optionsGrid.height + (ScreenTools.defaultFontPixelHeight * 4)
                    color:      qgcPal.window
                    radius:     ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    GridLayout {
                        id:                 optionsGrid
                        columns:            2
                        anchors.centerIn:   parent
                        anchors.margins:    ScreenTools.defaultFontPixelWidth  * 2
                        rowSpacing:         ScreenTools.defaultFontPixelWidth  * 1.5
                        columnSpacing:      ScreenTools.defaultFontPixelHeight * 2
                        QGCLabel {
                            text:       qsTr("Max Cache Disk Size (MB):")
                        }
                        QGCTextField {
                            id:             maxCacheSize
                            maximumLength:  6
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator {bottom: 1; top: 262144;}
                        }
                        QGCLabel {
                            text:       qsTr("Max Cache Memory Size (MB):")
                        }
                        QGCTextField {
                            id:             maxCacheMemSize
                            maximumLength:  4
                            inputMethodHints: Qt.ImhDigitsOnly
                            validator: IntValidator {bottom: 1; top: 4096;}
                        }
                        Item {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            height:             ScreenTools.defaultFontPixelHeight * 1.5
                            QGCLabel {
                                anchors.centerIn: parent
                                text: qsTr("Memory cache changes require a restart to take effect.")
                                font.pointSize: ScreenTools.smallFontPointSize
                            }
                        }
                        Rectangle {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            height:             1
                            color:              qgcPal.text
                        }
                        QGCLabel {
                            text: qsTr("MapBox Access Token")
                        }
                        QGCTextField {
                            id:                 mapBoxToken
                            maximumLength:      256
                            width:              ScreenTools.defaultFontPixelWidth * 30
                        }
                        Item {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            height:             ScreenTools.defaultFontPixelHeight * 1.5
                            QGCLabel {
                                anchors.centerIn: parent
                                text: qsTr("With an access token, you can use MapBox Maps.")
                                font.pointSize: ScreenTools.smallFontPointSize
                            }
                        }
                    }
                }
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: ScreenTools.defaultFontPixelWidth
                    QGCButton {
                        text:       qsTr("Save")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked:  {
                            QGroundControl.mapEngineManager.mapboxToken  = mapBoxToken.text
                            QGroundControl.mapEngineManager.maxDiskCache = parseInt(maxCacheSize.text)
                            QGroundControl.mapEngineManager.maxMemCache  = parseInt(maxCacheMemSize.text)
                            showList()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Cancel")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked:  {
                            showList()
                        }
                    }
                }
            }
        }
    }
}
