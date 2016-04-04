/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.1
import QtQuick.Layouts          1.2
import QtLocation               5.3
import QtPositioning            5.3

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Rectangle {
    id:                 _offlineMapRoot
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property var    _currentSelection: null

    property string mapKey:        "lastMapType"

    property string mapType:        QGroundControl.mapEngineManager.loadSetting(mapKey, "Google Street Map")
    property int    mapMargin:      (ScreenTools.defaultFontPixelHeight * 0.2).toFixed(0)
    property real   infoWidth:      Math.max(Math.max(nameLabel.width, descLabel.width), (ScreenTools.defaultFontPixelWidth * 40))
    property bool   isDefaultSet:   _offlineMapRoot._currentSelection && _offlineMapRoot._currentSelection.defaultSet

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

    function checkSanity() {
        if(QGroundControl.mapEngineManager.crazySize) {
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
        _tileSetList.visible = false
        _infoView.visible = false
        _mapView.visible = false
        _optionsView.visible = true
    }

    function showMap() {
        _tileSetList.visible = false
        _infoView.visible = false
        _mapView.visible = true
        _optionsView.visible = false
    }

    function showList() {
        _tileSetList.visible = true
        _infoView.visible = false
        _mapView.visible = false
        _optionsView.visible = false
    }

    function showInfo() {
        if(_currentSelection && !_offlineMapRoot._currentSelection.deleting) {
            _tileSetList.visible = false
            _mapView.visible = false
            _infoView.visible = true
            _optionsView.visible = false
        } else
            showList()
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    onMapTypeChanged: {
        updateMap()
        QGroundControl.mapEngineManager.saveSetting(mapKey, mapType)
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
                font.pixelSize: ScreenTools.mediumFontPixelSize
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCCheckBox {
                id:         showTilePreview
                text:       qsTr("Show tile min/max zoom level preview")
                checked:    false
                visible:    _mapView.visible
                anchors.verticalCenter: parent.verticalCenter
            }
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
            spacing:            (ScreenTools.defaultFontPixelHeight * 0.5).toFixed(0)
            anchors.horizontalCenter: parent.horizontalCenter

            OfflineMapButton {
                text:           qsTr("Add new set")
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         (ScreenTools.defaultFontPixelHeight * 2).toFixed(0)
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
                    height:         (ScreenTools.defaultFontPixelHeight * 2).toFixed(0)
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
    Rectangle {
        id:                 _mapView
        color:              qgcPal.window
        width:              parent.width
        anchors.top:        _offlineMapTopRect.bottom
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        visible:            false

        Rectangle {
            width:          parent.width
            anchors.top:    parent.top
            anchors.bottom: bottomRect.top
            color:          (qgcPal.globalTheme === QGCPalette.Light) ? "black" : "#98aca4"

            Map {
                id:                 _map
                anchors.fill:       parent
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.15
                center:             QGroundControl.defaultMapPosition
                gesture.flickDeceleration:  3000
                gesture.activeGestures:     MapGestureArea.ZoomGesture | MapGestureArea.PanGesture | MapGestureArea.FlickGesture
                plugin: Plugin { name: "QGroundControl" }

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

            Rectangle {
                width:              ScreenTools.defaultFontPixelHeight * 16
                height:             ScreenTools.defaultFontPixelHeight * 9
                anchors.top:        parent.top
                anchors.left:       parent.left
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                color:              "black"
                visible:            showTilePreview.checked
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
                visible:            showTilePreview.checked
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
        Rectangle {
            id:     bottomRect
            width:  parent.width
            height: _controlRow.height + (ScreenTools.defaultFontPixelHeight * 2)
            color:  qgcPal.window
            anchors.bottom: parent.bottom
            Row {
                id: _controlRow
                anchors.centerIn: parent
                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                Rectangle {
                    height:     _zoomRow.height + ScreenTools.defaultFontPixelHeight * 1.5
                    width:      _zoomRow.width  + ScreenTools.defaultFontPixelWidth
                    color:      "#98aca4"
                    border.color: "black"
                    border.width: 2
                    radius:     ScreenTools.defaultFontPixelWidth * 0.5
                    anchors.verticalCenter: parent.verticalCenter
                    Row {
                        id: _zoomRow
                        anchors.centerIn:   parent
                        Column {
                            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                            anchors.verticalCenter: parent.verticalCenter
                            Row {
                                spacing:        ScreenTools.defaultFontPixelWidth * 0.5
                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        text:   qsTr("Min")
                                        color:  "black"
                                        width:  ScreenTools.defaultFontPixelWidth * 5
                                        font.pixelSize: ScreenTools.smallFontPixelSize
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Label {
                                        text:   qsTr("Zoom")
                                        color:  "black"
                                        width:  ScreenTools.defaultFontPixelWidth * 5
                                        font.pixelSize: ScreenTools.smallFontPixelSize
                                        horizontalAlignment: Text.AlignHCenter
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
                                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 12
                                            implicitHeight: 4
                                            color:          "gray"
                                            radius:         4
                                        }
                                        handle: Rectangle {
                                            anchors.centerIn: parent
                                            color: control.pressed ? "white" : "lightgray"
                                            border.color: "gray"
                                            border.width:   2
                                            implicitWidth:  ScreenTools.isAndroid ? 60 : 30
                                            implicitHeight: ScreenTools.isAndroid ? 60 : 30
                                            radius:         10
                                            Label {
                                                text:  _slider0.value
                                                anchors.centerIn: parent
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
                                        text:   qsTr("Max")
                                        color:  "black"
                                        width:  ScreenTools.defaultFontPixelWidth * 5
                                        font.pixelSize: ScreenTools.smallFontPixelSize
                                        horizontalAlignment: Text.AlignHCenter
                                    }
                                    Label {
                                        text:   qsTr("Zoom")
                                        color:  "black"
                                        width:  ScreenTools.defaultFontPixelWidth * 5
                                        font.pixelSize: ScreenTools.smallFontPixelSize
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
                                            implicitWidth:  ScreenTools.defaultFontPixelWidth * 12
                                            implicitHeight: 4
                                            color:          "gray"
                                            radius:         4
                                        }
                                        handle: Rectangle {
                                            anchors.centerIn: parent
                                            color: control.pressed ? "white" : "lightgray"
                                            border.color: "gray"
                                            border.width:   2
                                            implicitWidth:  ScreenTools.isAndroid ? 60 : 30
                                            implicitHeight: ScreenTools.isAndroid ? 60 : 30
                                            radius:         10
                                            Label {
                                                text:  _slider1.value
                                                anchors.centerIn: parent
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
                                text:   qsTr("Tile Count")
                                color:  "black"
                                width:  ScreenTools.defaultFontPixelWidth * 12
                                font.pixelSize: ScreenTools.smallFontPixelSize
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:  QGroundControl.mapEngineManager.tileCountStr
                                color: "black"
                                width: ScreenTools.defaultFontPixelWidth * 12
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:   qsTr("Set Size (Est)")
                                color:  "black"
                                width:  ScreenTools.defaultFontPixelWidth * 12
                                font.pixelSize: ScreenTools.smallFontPixelSize
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Label {
                                text:  QGroundControl.mapEngineManager.tileSizeStr
                                color: "black"
                                width: ScreenTools.defaultFontPixelWidth * 12
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                    Row {
                        spacing:        ScreenTools.defaultFontPixelWidth * 2
                        QGCLabel {
                            text:   qsTr("Name:")
                            width:  ScreenTools.defaultFontPixelWidth * 10
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCTextField {
                            id:     setName
                            width:  ScreenTools.defaultFontPixelWidth * 24
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 2
                        QGCLabel {
                            text:  qsTr("Description:")
                            width:  ScreenTools.defaultFontPixelWidth * 10
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCTextField {
                            id:     setDescription
                            text:   qsTr("Description")
                            width:  ScreenTools.defaultFontPixelWidth * 24
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth * 2
                        QGCLabel {
                            text:  qsTr("Map Type:")
                            width:  ScreenTools.defaultFontPixelWidth * 10
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignRight
                        }
                        QGCComboBox {
                            id:         mapCombo
                            width:      ScreenTools.defaultFontPixelWidth * 24
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
                    width:  ScreenTools.defaultFontPixelWidth * 1.5
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
    Rectangle {
        id:                 _infoView
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
                    width:      infoWidth
                    height:     nameLabel.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color:      qgcPal.window
                    radius:     ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:     nameLabel
                        text:   _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.name : ""
                        font.pixelSize:   ScreenTools.isAndroid ? ScreenTools.mediumFontPixelSize : ScreenTools.largeFontPixelSize
                        anchors.centerIn: parent
                    }
                }
                QGCLabel {
                    id:     descLabel
                    text:   _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.description : ""
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Rectangle {
                    id:         infoRect
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
                            text:       qsTr("Map Type:")
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.mapTypeStr : ""
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       qsTr("Min Zoom:")
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.minZoom : ""
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       qsTr("Max Zoom:")
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.maxZoom : ""
                            visible:    !isDefaultSet
                        }
                        QGCLabel {
                            text:       isDefaultSet ? qsTr("Default Set Size:") : qsTr("Total Size:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.tilesSizeStr : ""
                        }
                        QGCLabel {
                            text:       isDefaultSet ? qsTr("Default Set Tile Count:") : qsTr("Total Tile Count:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.numTilesStr : ""
                        }
                        QGCLabel {
                            text:       isDefaultSet ? qsTr("Total Size (All Sets):") : qsTr("Downloaded Size:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedSizeStr : ""
                        }
                        QGCLabel {
                            text:       isDefaultSet ? qsTr("Total Count (All Sets):") : qsTr("Downloaded Count:")
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.savedTilesStr : ""
                        }
                        QGCLabel {
                            text:       qsTr("Error Count:")
                            visible:    !isDefaultSet && _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                        }
                        QGCLabel {
                            text:       _offlineMapRoot._currentSelection ? _offlineMapRoot._currentSelection.errorCountStr : ""
                            visible:    !isDefaultSet && _offlineMapRoot._currentSelection && !_offlineMapRoot._currentSelection.complete
                        }
                    }
                }
                Item {
                    height:     ScreenTools.defaultFontPixelHeight * 0.5
                    width:      1
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
                                    if(_offlineMapRoot._currentSelection.defaultSet)
                                        return blurb + qsTr("\nNote that deleteting the Default Set deletes all tiles from all sets.")
                                    else
                                        return blurb
                                }
                                return ""
                            }
                            onYes: {
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
                        visible:    !isDefaultSet && _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.complete && !_offlineMapRoot._currentSelection.downloading)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                _offlineMapRoot._currentSelection.resumeDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Cancel Download")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        enabled:    _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.deleting && _offlineMapRoot._currentSelection.downloading)
                        visible:    !isDefaultSet && _offlineMapRoot._currentSelection && (!_offlineMapRoot._currentSelection.complete && _offlineMapRoot._currentSelection.downloading)
                        onClicked: {
                            if(_offlineMapRoot._currentSelection)
                                _offlineMapRoot._currentSelection.cancelDownloadTask()
                        }
                    }
                    QGCButton {
                        text:       qsTr("Back")
                        width:      ScreenTools.defaultFontPixelWidth * 18
                        onClicked:  showList()
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
                        font.pixelSize:     ScreenTools.isAndroid ? ScreenTools.mediumFontPixelSize : ScreenTools.largeFontPixelSize
                        anchors.centerIn:   parent
                    }
                }
                Rectangle {
                    id:         optionsRect
                    width:      optionsGrid.width  + (ScreenTools.defaultFontPixelWidth * 4)
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
                            implicitHeight:     ScreenTools.defaultFontPixelHeight * 1.5
                            QGCLabel {
                                anchors.centerIn: parent
                                text: qsTr("Memory cache changes require a restart to take effect.")
                                font.pixelSize: ScreenTools.defaultFontPixelSize * 0.85
                            }
                        }
                        Rectangle {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            implicitHeight:     1
                            color:              qgcPal.text
                        }
                        QGCLabel {
                            text: qsTr("MapBox Access Token")
                        }
                        QGCTextField {
                            id:                 mapBoxToken
                            Layout.fillWidth:   true
                            maximumLength:      256
                            implicitWidth :     ScreenTools.defaultFontPixelWidth * 30
                        }
                        Item {
                            Layout.columnSpan:  2
                            Layout.fillWidth:   true
                            implicitHeight:     ScreenTools.defaultFontPixelHeight * 1.5
                            QGCLabel {
                                anchors.centerIn: parent
                                text: qsTr("With an access token, you can use MapBox Maps.")
                                font.pixelSize: ScreenTools.defaultFontPixelSize * 0.85
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
